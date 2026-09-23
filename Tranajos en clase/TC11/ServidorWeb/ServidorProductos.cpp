/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *
  *  TicAmazon - Bodega (ServidorProductos), sobre el Almacenamiento nuevo
  *
  *  Reemplaza el FileSystem anterior (superbloque/bitmap/indices binarios)
  *  por el Almacenamiento nuevo (un solo archivo, bloques de 256 bytes,
  *  bloque de control + directorio de bodegas + bloques de datos con
  *  registros de texto separados por comas). Sigue hablando exactamente el
  *  mismo protocolo mancomunado con el Intermediario (PEDIR:CAT, etc. --
  *  ORIGEN|DESTINO/TIPO/campos); lo unico que cambia es el contenedor.
  *
  *  Diferencia de diseno importante: como el Almacenamiento identifica todo
  *  por NOMBRE (bodega, categoria, producto -- todo string), ya no hace
  *  falta la tabla catalogoIds (nombre de categoria -> id numerico) que
  *  usabamos para traducir hacia el FileSystem anterior. Se simplifica.
  *
  *  "Al menos dos bodegas": un solo proceso, un solo archivo
  *  (almacenamiento.data), con DOS bodegas nombradas adentro
  *  ("Bodega-1", "Bodega-2"), cada una con sus propias categorias y
  *  productos. Las solicitudes de categoria/producto se resuelven
  *  recorriendo TODAS las bodegas del archivo (asi el Intermediario ve un
  *  catalogo unificado sin necesitar saber en cual bodega esta cada cosa).
  *
  *  Nota de alcance: el Almacenamiento nuevo no tiene un indice de ordenes
  *  (a diferencia del FileSystem anterior, que si persistia cada factura
  *  con crearOrden/leerOrden). La factura se sigue armando correctamente
  *  -- el Intermediario ya la arma en memoria con los items confirmados
  *  via RESERVE_STOCK -- pero esta version no la vuelve a guardar en disco
  *  como un registro de orden permanente.
  *
 **/

#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <vector>
#include <cstring>
#include <csignal>
#include <chrono>

#include "Socket.hpp"
#include "almacenamiento.hpp"
#include "Logger.hpp"
#include "Protocolo.hpp"
#include "Descubrimiento.hpp"

#define BUFSIZE 512

static Almacenamiento almacen;
static std::mutex almacenMutex;
static Logger * bitacora = nullptr;
static Descubrimiento * descubrimiento = nullptr;
static VSocket * s1Global = nullptr;


/**
  *  SembrarSiEsNueva
  *     Si el archivo se acaba de crear, siembra dos bodegas de ejemplo con
  *     categorias y productos distintos (para la prueba de "al menos dos
  *     bodegas").
  *
 **/
void SembrarSiEsNueva( bool archivoEraNuevo ) {

   if ( !archivoEraNuevo ) return;

   almacen.crear_bodega( "Bodega-1" );
   almacen.insertar_producto( "Bodega-1", "Alimentos", "CafeFrio", "40", "3.25" );
   almacen.insertar_producto( "Bodega-1", "Alimentos", "Tresleches", "15", "3.80" );
   almacen.insertar_producto( "Bodega-1", "Bloques", "BloqueRojo", "120", "3.00" );

   almacen.crear_bodega( "Bodega-2" );
   almacen.insertar_producto( "Bodega-2", "Vehiculos", "RuedaChica", "60", "4.00" );
   almacen.insertar_producto( "Bodega-2", "Reposteria", "PastelChoco", "12", "8.50" );
   almacen.insertar_producto( "Bodega-2", "Reposteria", "FlanCaramelo", "20", "3.00" );

   std::cout << "[BODEGA] Archivo nuevo: se sembraron Bodega-1 y Bodega-2 con productos de ejemplo\n";

}


/**
  *  BuscarProducto
  *     Recorre TODAS las bodegas del archivo buscando un producto por
  *     nombre. Devuelve tambien en que bodega vive (necesario para poder
  *     llamar actualizar_cantidad despues).
  *
 **/
bool BuscarProducto( const std::string & nombreProducto, ProductoTexto * out, std::string * bodegaOut ) {

   for ( auto & bodega : almacen.listar_bodegas() ) {
      for ( auto & p : almacen.listar_productos( bodega ) ) {
         if ( p.producto == nombreProducto ) {
            *out = p;
            *bodegaOut = bodega;
            return true;
         }
      }
   }
   return false;

}


// ===== manejadores del protocolo (mismos codigos/formato que antes; cambia solo el contenedor) =====

std::string ManejarCat() {

   std::lock_guard<std::mutex> guard( almacenMutex );

   std::vector<std::string> categoriasUnicas;
   for ( auto & bodega : almacen.listar_bodegas() ) {
      for ( auto & p : almacen.listar_productos( bodega ) ) {
         bool yaEsta = false;
         for ( auto & c : categoriasUnicas ) if ( c == p.categoria ) { yaEsta = true; break; }
         if ( !yaEsta ) categoriasUnicas.push_back( p.categoria );
      }
   }

   std::ostringstream lista;
   for ( size_t i = 0; i < categoriasUnicas.size(); i++ ) {
      if ( i > 0 ) lista << ",";
      lista << categoriasUnicas[ i ];
   }

   return construirMensaje( "SERV", "INT", TipoMensaje::CATEGORY_LIST_INT, { lista.str() } );

}

std::string ManejarProd( const std::string & categoria, const std::string & origen ) {

   std::lock_guard<std::mutex> guard( almacenMutex );

   std::ostringstream lista;
   int count = 0;

   for ( auto & bodega : almacen.listar_bodegas() ) {
      for ( auto & p : almacen.listar_productos( bodega ) ) {
         if ( p.categoria != categoria ) continue;
         if ( count > 0 ) lista << ";";
         lista << p.producto << "," << p.precio << "," << p.cantidad;
         count++;
      }
   }

   if ( 0 == count ) {
      return construirMensaje( "SERV", origen, TipoMensaje::PRODUCT_LIST_B, { "0" } );
   }

   return construirMensaje( "SERV", origen, TipoMensaje::PRODUCT_LIST_B, { std::to_string( count ), lista.str() } );

}

std::string ManejarDetalle( const std::string & nombreProducto, const std::string & origen ) {

   std::lock_guard<std::mutex> guard( almacenMutex );

   ProductoTexto p;
   std::string bodega;
   if ( !BuscarProducto( nombreProducto, &p, &bodega ) ) {
      return construirMensaje( "SERV", origen, TipoMensaje::PRODUCT_NOT_FOUND, { nombreProducto } );
   }

   return construirMensaje( "SERV", origen, TipoMensaje::PRODUCT_DETAIL,
      { p.producto, p.precio, p.cantidad, "Producto de " + bodega } );

}

std::string ManejarReservar( const std::string & nombreProducto, const std::string & cantidadStr, const std::string & origen ) {

   std::lock_guard<std::mutex> guard( almacenMutex );

   ProductoTexto p;
   std::string bodega;
   if ( !BuscarProducto( nombreProducto, &p, &bodega ) ) {
      return construirMensaje( "SERV", origen, TipoMensaje::PRODUCT_NOT_FOUND, { nombreProducto } );
   }

   int cantidad = std::stoi( cantidadStr );
   std::string error;
   if ( !almacen.actualizar_cantidad( bodega, nombreProducto, -cantidad, &error ) ) {
      return construirMensaje( "SERV", origen, TipoMensaje::PRODUCT_NOT_FOUND, { nombreProducto } );
   }

   return construirMensaje( "SERV", origen, TipoMensaje::PRODUCT_DETAIL, { nombreProducto, "reservado", cantidadStr } );

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
   std::string respuesta;

   if ( TipoMensaje::REQUEST_CATEGORY == m.tipo ) {

      respuesta = ManejarCat();

   } else if ( TipoMensaje::REQUEST_PRODUCTS_B == m.tipo ) {

      std::string categoria = m.campos.empty() ? "" : m.campos[ 0 ];
      respuesta = ManejarProd( categoria, m.origen );

   } else if ( TipoMensaje::REQUEST_DETAIL_B == m.tipo ) {

      std::string producto = m.campos.empty() ? "" : m.campos[ 0 ];
      respuesta = ManejarDetalle( producto, m.origen );

   } else if ( TipoMensaje::RESERVE_STOCK == m.tipo ) {

      std::string producto = m.campos.size() > 0 ? m.campos[ 0 ] : "";
      std::string cantidad = m.campos.size() > 1 ? m.campos[ 1 ] : "";
      respuesta = ManejarReservar( producto, cantidad, m.origen );

   } else if ( TipoMensaje::DEATH == m.tipo ) {

      // (no deberia llegar por TCP; DEATH es UDP/multicast, pero se ignora sin error si llega)
      respuesta = "";

   } else {

      respuesta = construirError90( "SERV", m.origen, m.tipo, "tipo_mensaje", std::to_string( m.tipo ) );

   }

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
   almacen.cerrar_archivo();
   if ( nullptr != s1Global ) delete s1Global;
   exit( 0 );

}


int main( int argc, char ** argv ) {

   int puertoTCP = ( argc > 1 ) ? atoi( argv[ 1 ] ) : 9091;
   std::string archivoData = ( argc > 2 ) ? argv[ 2 ] : "almacenamiento.data";
   int puertoMulticast = ( argc > 3 ) ? atoi( argv[ 3 ] ) : 5000;

   bitacora = new Logger( "./bitacora_bodega_" + std::to_string( puertoTCP ) + ".log" );

   std::ifstream pruebaExiste( archivoData );
   bool archivoEraNuevo = !pruebaExiste.good();
   pruebaExiste.close();

   if ( archivoEraNuevo ) {
      almacen.crear_archivo( archivoData );
   } else {
      almacen.abrir_archivo( archivoData );
   }

   SembrarSiEsNueva( archivoEraNuevo );

   VSocket * s1 = new Socket( 's' );
   s1Global = s1;
   s1->Bind( puertoTCP );
   s1->MarkPassive( 10 );

   descubrimiento = new Descubrimiento( "SERV", "127.0.0.1", puertoTCP, puertoMulticast );
   descubrimiento->AnunciarPresencia();

   std::thread hiloHeartbeat( [] () {
      while ( true ) {
         std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
         if ( nullptr != descubrimiento ) descubrimiento->AnunciarPresencia();
      }
   } );
   hiloHeartbeat.detach();

   signal( SIGINT, ManejarCierre );

   std::cout << "[BODEGA] Escuchando en puerto " << puertoTCP << " (archivo: " << archivoData << ")\n";
   std::cout << "[BODEGA] Bodegas en el archivo: ";
   for ( auto & b : almacen.listar_bodegas() ) std::cout << b << " ";
   std::cout << "\n";
   bitacora->log( "Bodega iniciada en el puerto " + std::to_string( puertoTCP ), ServidorProductos );

   for ( ; ; ) {
      VSocket * intermediario = s1->AcceptConnection();
      std::thread worker( task, intermediario );
      worker.detach();
   }

   return 0;

}
