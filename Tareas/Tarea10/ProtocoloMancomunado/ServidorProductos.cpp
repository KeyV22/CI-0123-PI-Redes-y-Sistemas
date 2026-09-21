/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *
  *  TicAmazon - Bodega (ServidorProductos), protocolo mancomunado
  *
  *  Decisiones de diseno de esta version:
  *
  *    - Modelo de atencion: HILOS (un std::thread por conexion aceptada),
  *      protegiendo el FileSystem con un mutex -- igual que la version HTTP
  *      anterior, por la misma razon: una sola instancia de bodega
  *      compartida entre todas las solicitudes.
  *
  *    - Modelo del contenedor: el FileSystem propio ya documentado
  *      (bloques de 256 bytes, superbloque, bitmap, indices encadenables).
  *      NO se modifico su formato para esta entrega.
  *
  *    - Identificacion de productos: el documento del protocolo identifica
  *      productos por NOMBRE (string) en todos sus mensajes, pero nuestro
  *      FileSystem los indexa por idProducto (numero). Se agrega aqui una
  *      busqueda por nombre (recorre las categorias conocidas) que resuelve
  *      el nombre al id real antes de llamar a fs.reservarProducto(), etc.
  *
  *    - Campo "descripcion": el struct Producto de nuestro FileSystem no
  *      tiene un campo de descripcion (no se modifico el formato en disco
  *      ya documentado para no invalidar el diseno ya entregado). Se
  *      sintetiza una descripcion generica en esta capa solo para poder
  *      completar el campo que exige el mensaje PRODUCT_DETAIL (25).
  *
  *    - Precio: el documento pide formato decimal NNNNN.NN. El FileSystem
  *      guarda el precio como entero (centavos), igual que ya se hacia en
  *      las versiones anteriores del proyecto; aqui se formatea a decimal
  *      dividiendo entre 100 antes de mandarlo por el protocolo.
  *
  *
 **/

#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <cstring>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <chrono>

#include "Socket.h"
#include "FileSystem.hpp"
#include "Logger.hpp"
#include "Protocolo.hpp"
#include "Descubrimiento.hpp"

#define BUFSIZE 512   // el documento limita cada mensaje a 256 bytes; dejamos margen

static FileSystem * fsPtr = nullptr;
#define fs (*fsPtr)
static std::mutex fsMutex;
static Logger * bitacora = nullptr;
static Descubrimiento * descubrimiento = nullptr;
static VSocket * s1Global = nullptr;

static std::map<std::string, uint8_t> catalogoIds;   // nombre de categoria -> id numerico interno


/**
  *  FormatearPrecio
  *     Convierte el precio interno (entero, centavos) al formato decimal
  *     NNNNN.NN que exige el documento del protocolo.
  *
 **/
std::string FormatearPrecio( uint32_t centavos ) {
   std::ostringstream out;
   out << std::fixed << std::setprecision( 2 ) << ( centavos / 100.0 );
   return out.str();
}


/**
  *  BuscarProductoPorNombre
  *     El protocolo identifica productos por nombre; el FileSystem los
  *     indexa por id. Recorre las categorias conocidas buscando el nombre.
  *
 **/
bool BuscarProductoPorNombre( const std::string & nombre, Producto * out, uint8_t * idCategoriaOut = nullptr ) {

   for ( auto & entrada : catalogoIds ) {
      for ( auto & p : fs.listarProductos( entrada.second ) ) {
         if ( std::string( p.nombre ) == nombre ) {
            *out = p;
            if ( nullptr != idCategoriaOut ) *idCategoriaOut = entrada.second;
            return true;
         }
      }
   }
   return false;

}


/**
  *  SembrarCatalogo
  *     Cada bodega puede sembrar un catalogo distinto (para la prueba de
  *     "al menos dos bodegas con productos") segun el parametro "variante".
  *
 **/
void SembrarCatalogo( bool archivoEraNuevo, int variante ) {

   if ( !archivoEraNuevo ) return;

   auto crear = []( uint32_t id, const char * nombre, uint32_t precioCentavos, uint32_t stock ) {
      Producto p{};
      p.idProducto = id;
      p.precio = precioCentavos;
      p.cantidadDisponible = stock;
      memset( p.nombre, 0, 20 );
      strncpy( p.nombre, nombre, 19 );
      return p;
   };

   if ( 1 == variante ) {

      catalogoIds = { { "Alimentos", 1 }, { "Pesados", 2 } };
      fs.agregarProducto( 1, "Alimentos", crear( 101, "CafeNiFrioNiCaliente", 325, 40 ) );
      fs.agregarProducto( 1, "Alimentos", crear( 102, "Cuatroleches", 380, 15 ) );
      fs.agregarProducto( 2, "Bloques", crear( 201, "SalSol", 300, 120 ) );
      std::cout << "[BODEGA] Catalogo variante 1 (Alimentos/Bloques) sembrado\n";

   } else {

      catalogoIds = { { "Vehiculos", 3 }, { "Reposteria", 4 } };
      fs.agregarProducto( 3, "Vehiculos", crear( 301, "Hotwheel", 400, 60 ) );
      fs.agregarProducto( 4, "Reposteria", crear( 401, "Pastelito", 850, 12 ) );
      fs.agregarProducto( 4, "Reposteria", crear( 402, "Flan", 300, 20 ) );
      std::cout << "[BODEGA] Catalogo variante 2 (Vehiculos/Reposteria) sembrado\n";

   }

}


/**
  *  CargarCatalogoIds
  *     Si la bodega YA existia en disco (no es la primera corrida), hay que
  *     reconstruir el mapa nombre->id recorriendo las categorias reales
  *     que quedaron guardadas (SembrarCatalogo solo llena el mapa la
  *     primera vez que se crea el archivo).
  *
 **/
void CargarCatalogoIds( int variante ) {

   if ( !catalogoIds.empty() ) return;   // ya se lleno en SembrarCatalogo

   if ( 1 == variante ) {
      catalogoIds = { { "Alimentos", 1 }, { "Pesados", 2 } };
   } else {
      catalogoIds = { { "Vehiculos", 3 }, { "Reposteria", 4 } };
   }

}


// ===== Manejo de cada mensaje del protocolo mancomunado (capa Intermediario<->Bodega) =====

std::string ProcesarMensaje( const MensajeV2 & m ) {

   ResultadoValidacion r;

   if ( TipoMensaje::REQUEST_CATEGORY == m.tipo ) {   // 10: listar categorias (segun tabla seccion b, sin conflicto con los flujos)

      std::lock_guard<std::mutex> guard( fsMutex );

      std::ostringstream lista;
      bool primero = true;
      for ( auto & entrada : catalogoIds ) {
         if ( !primero ) lista << ",";
         lista << entrada.first;
         primero = false;
      }

      return construirMensaje( "SERV", m.origen, TipoMensaje::CATEGORY_LIST_INT, { lista.str() } );

   } else if ( TipoMensaje::REQUEST_PRODUCTS_B == m.tipo ) {   // 21: solicitar productos de una categoria

      std::string categoria = m.campos.empty() ? "" : m.campos[ 0 ];

      if ( !ValidarCampoPorTipo( "categoria", categoria, &r ) ) {
         return r.esErrorTamano
              ? construirError91( "SERV", m.origen, m.tipo, "categoria", r.tamanoRecibido, r.tamanoMaximo )
              : construirError90( "SERV", m.origen, m.tipo, "categoria", categoria );
      }

      std::lock_guard<std::mutex> guard( fsMutex );

      auto it = catalogoIds.find( categoria );
      if ( catalogoIds.end() == it ) {
         return construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_LIST_B, { "0" } );
      }

      auto productos = fs.listarProductos( it->second );
      std::ostringstream lista;
      for ( size_t i = 0; i < productos.size(); i++ ) {
         lista << productos[ i ].nombre << "," << FormatearPrecio( productos[ i ].precio )
               << "," << productos[ i ].cantidadDisponible;
         if ( i + 1 < productos.size() ) lista << ";";
      }

      return construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_LIST_B,
                                { std::to_string( productos.size() ), lista.str() } );

   } else if ( TipoMensaje::REQUEST_DETAIL_B == m.tipo ) {   // 24: detalle de un producto

      std::string nombre = m.campos.empty() ? "" : m.campos[ 0 ];

      if ( !ValidarCampoPorTipo( "producto", nombre, &r ) ) {
         return r.esErrorTamano
              ? construirError91( "SERV", m.origen, m.tipo, "producto", r.tamanoRecibido, r.tamanoMaximo )
              : construirError90( "SERV", m.origen, m.tipo, "producto", nombre );
      }

      std::lock_guard<std::mutex> guard( fsMutex );

      Producto p;
      if ( BuscarProductoPorNombre( nombre, &p ) ) {
         return construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_DETAIL,
            { p.nombre, FormatearPrecio( p.precio ), std::to_string( p.cantidadDisponible ),
              "Producto de la bodega TicAmazon" } );   // descripcion sintetica (ver nota de diseno arriba)
      }

      return construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_NOT_FOUND, { nombre } );

   } else if ( TipoMensaje::RESERVE_STOCK == m.tipo ) {   // 50: reservar stock

      std::string nombre = m.campos.size() > 0 ? m.campos[ 0 ] : "";
      std::string cantStr = m.campos.size() > 1 ? m.campos[ 1 ] : "";

      if ( !ValidarCampoPorTipo( "producto", nombre, &r ) ) {
         return construirError90( "SERV", m.origen, m.tipo, "producto", nombre );
      }
      if ( !ValidarCampoPorTipo( "count", cantStr, &r ) ) {
         return construirError90( "SERV", m.origen, m.tipo, "cantidad", cantStr );
      }

      std::lock_guard<std::mutex> guard( fsMutex );

      Producto p;
      if ( !BuscarProductoPorNombre( nombre, &p ) ) {
         return construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_NOT_FOUND, { nombre } );
      }

      std::string error;
      uint32_t cantidad = (uint32_t) std::stoul( cantStr );
      if ( fs.reservarProducto( p.idProducto, cantidad, &error ) ) {
         return construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_DETAIL,
            { p.nombre, "reservado", cantStr } );
      }

      return construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_NOT_FOUND, { nombre } );

   }

   return construirError90( "SERV", m.origen, m.tipo, "tipo_mensaje", std::to_string( m.tipo ) );

}


void task( VSocket * intermediario ) {

   char buffer[ BUFSIZE ] = { 0 };
   size_t leidos = intermediario->Read( buffer, BUFSIZE - 1 );
   buffer[ leidos ] = '\0';

   std::string linea( buffer );
   size_t fin = linea.find_first_of( "\r\n" );
   if ( std::string::npos != fin ) linea = linea.substr( 0, fin );

   bitacora->log( linea, ServidorProductos );

   MensajeV2 m = parsearMensaje( linea );
   std::string respuesta = ProcesarMensaje( m );

   bitacora->log( respuesta, ServidorProductos );

   intermediario->Write( respuesta.c_str() );
   intermediario->Close();
   delete intermediario;

}


void ManejarCierre( int senal ) {

   std::cout << "\n[BODEGA] Cerrando ordenadamente...\n";
   if ( nullptr != descubrimiento ) {
      descubrimiento->AnunciarMuerte();
      bitacora->log( "DEATH enviado antes de cerrar", ServidorProductos );
   }
   if ( nullptr != s1Global ) {
      delete s1Global;
   }
   exit( 0 );

}


int main( int argc, char ** argv ) {

   int puertoTCP = ( argc > 1 ) ? atoi( argv[ 1 ] ) : 9091;
   std::string archivoDat = ( argc > 2 ) ? argv[ 2 ] : "bodega1.dat";
   int variante = ( argc > 3 ) ? atoi( argv[ 3 ] ) : 1;
   int puertoMulticast = ( argc > 4 ) ? atoi( argv[ 4 ] ) : 5000;

   bitacora = new Logger( "./bitacora_bodega_" + std::to_string( puertoTCP ) + ".log" );

   std::ifstream pruebaExiste( archivoDat );
   bool archivoEraNuevo = !pruebaExiste.good();
   pruebaExiste.close();

   fsPtr = new FileSystem( archivoDat );
   SembrarCatalogo( archivoEraNuevo, variante );
   CargarCatalogoIds( variante );

   VSocket * s1 = new Socket( 's' );
   s1Global = s1;
   s1->Bind( puertoTCP );
   s1->MarkPassive( 10 );

   descubrimiento = new Descubrimiento( "SERV", "127.0.0.1", puertoTCP, puertoMulticast );
   descubrimiento->AnunciarPresencia();

   // Reanuncio periodico (heartbeat): si el Intermediario arranca DESPUES de
   // esta bodega, un solo ANNOUNCE inicial se perderia (multicast no "guarda"
   // mensajes para quien se une tarde al grupo). Reanunciar cada pocos
   // segundos garantiza que cualquier Intermediario que arranque despues
   // igual descubra esta bodega en poco tiempo.
   std::thread hiloHeartbeat( [] () {
      while ( true ) {
         std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
         if ( nullptr != descubrimiento ) descubrimiento->AnunciarPresencia();
      }
   } );
   hiloHeartbeat.detach();

   signal( SIGINT, ManejarCierre );

   std::cout << "[BODEGA] Escuchando en puerto " << puertoTCP << " (archivo: " << archivoDat
             << ", variante " << variante << ")\n";
   bitacora->log( "Bodega iniciada en el puerto " + std::to_string( puertoTCP ), ServidorProductos );

   for ( ; ; ) {
      VSocket * intermediario = s1->AcceptConnection();
      std::thread worker( task, intermediario );
      worker.detach();
   }

   return 0;

}
