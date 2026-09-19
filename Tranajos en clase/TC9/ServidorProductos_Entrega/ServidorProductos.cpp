/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  TicAmazon - Servidor de productos (bodega)
  *
  *  Este proceso ya NO habla HTTP con nadie: es un servicio interno que
  *  solo el "Intermediario" conoce. Expone el protocolo propio ya definido
  *  para el proyecto:
  *
  *     PEDIR:CAT                                -> lista de categorias
  *     PEDIR:PROD:<nombreCategoria>              -> productos de esa categoria
  *     AGREGAR:<idProducto>:<cantidad>           -> reserva stock
  *     PEDIR:FACTURA:<idProd>:<cant>,<idProd>:<cant>,...  -> genera la orden
  *
  *  Formato de respuesta (texto plano, una conexion por solicitud, el
  *  servidor cierra la conexion al terminar de responder):
  *
  *     PEDIR:CAT      -> "OK\n" + una categoria por linea
  *     PEDIR:PROD     -> "OK\n" + "id;nombre;precio;stock" por linea
  *                       o "ERR:<mensaje>\n"
  *     AGREGAR        -> "OK:<cantidad>\n" o "ERR:<mensaje>\n"
  *     PEDIR:FACTURA  -> "OK:<idOrden>\n" + "nombre;cantidad;precio;subtotal"
  *                       por linea + "TOTAL:<monto>\n"
  *
  *  Modelo de atencion: hilos (igual que la version anterior), protegiendo
  *  el file system con un mutex.
  *
 **/

#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <fstream>

#include "Socket.h"
#include "FileSystem.hpp"
#include "Logger.hpp"

#define PORT 9090
#define BUFSIZE 2048

static FileSystem * fsPtr = nullptr;
#define fs (*fsPtr)
static std::mutex fsMutex;
static Logger bitacora("./bitacora_productos.log");

static std::map<std::string, uint8_t> catalogoIds = {
   { "Alimentos",  1 },
   { "Bloques",    2 },
   { "Vehiculos",  3 }
};

void SembrarCatalogoSiEsNuevo( bool archivoEraNuevo ) {

   if ( !archivoEraNuevo ) {
      return;
   }

   auto crear = []( uint32_t id, const char * nombre, uint32_t precio, uint32_t stock ) {
      Producto p{};
      p.idProducto = id;
      p.precio = precio;
      p.cantidadDisponible = stock;
      memset( p.nombre, 0, 20 );
      strncpy( p.nombre, nombre, 19 );
      return p;
   };

   fs.agregarProducto( 1, "Alimentos", crear( 101, "Leche tres pinos", 650, 40 ) );
   fs.agregarProducto( 1, "Alimentos", crear( 102, "Bolsa de yiguirros", 900, 25 ) );
   fs.agregarProducto( 1, "Alimentos", crear( 103, "Barra de pan", 750, 30 ) );

   fs.agregarProducto( 2, "Bloques", crear( 201, "4x4 turbo motores", 300, 120 ) );
   fs.agregarProducto( 2, "Bloques", crear( 202, "fiuuuumba 300", 250, 95 ) );
   fs.agregarProducto( 2, "Bloques", crear( 203, "ALo con pollo", 1200, 15 ) );

   fs.agregarProducto( 3, "Vehiculos", crear( 301, "Atun sardimal", 400, 60 ) );
   fs.agregarProducto( 3, "Vehiculos", crear( 302, "Pinto Fresco", 1500, 20 ) );
   fs.agregarProducto( 3, "Vehiculos", crear( 303, "Salchichon el potro", 2200, 10 ) );

   std::cout << "[BODEGA] Catalogo de ejemplo sembrado (bodega nueva)\n";

}


// --- manejadores del protocolo (PEDIR:CAT / PEDIR:PROD / AGREGAR / PEDIR:FACTURA) ---

std::string ManejarCat() {

   std::lock_guard<std::mutex> guard( fsMutex );

   std::string resp = "OK\n";
   for ( auto & nombre : fs.listarCategorias() ) {
      resp += nombre + "\n";
   }
   return resp;

}

std::string ManejarProd( const std::string & nombreCategoria ) {

   auto it = catalogoIds.find( nombreCategoria );
   if ( catalogoIds.end() == it ) {
      return "ERR:La categoria \"" + nombreCategoria + "\" no existe.\n";
   }

   std::lock_guard<std::mutex> guard( fsMutex );

   std::string resp = "OK\n";
   for ( auto & p : fs.listarProductos( it->second ) ) {
      resp += std::to_string( p.idProducto ) + ";" + std::string( p.nombre ) + ";" +
              std::to_string( p.precio ) + ";" + std::to_string( p.cantidadDisponible ) + "\n";
   }
   return resp;

}

std::string ManejarAgregar( uint32_t idProducto, uint32_t cantidad ) {

   std::lock_guard<std::mutex> guard( fsMutex );

   std::string error;
   bool ok = fs.reservarProducto( idProducto, cantidad, &error );

   if ( ok ) {
      return "OK:" + std::to_string( cantidad ) + "\n";
   }
   return "ERR:" + error + "\n";

}

std::string ManejarFactura( const std::string & itemsCrudo ) {

   std::vector<ItemOrden> items;
   std::stringstream ss( itemsCrudo );
   std::string par;

   while ( std::getline( ss, par, ',' ) ) {
      size_t sep = par.find( ':' );
      if ( std::string::npos == sep ) continue;
      uint32_t idProd = (uint32_t) std::stoul( par.substr( 0, sep ) );
      uint16_t cant   = (uint16_t) std::stoul( par.substr( sep + 1 ) );
      items.push_back( { idProd, cant, 0 } );
   }

   if ( items.empty() ) {
      return "ERR:No se especificaron productos.\n";
   }

   std::lock_guard<std::mutex> guard( fsMutex );

   std::string resp;
   long total = 0;
   for ( auto & item : items ) {
      uint32_t precio = 0;
      std::string nombreProd = "(desconocido)";
      for ( auto & catEntry : catalogoIds ) {
         for ( auto & p : fs.listarProductos( catEntry.second ) ) {
            if ( p.idProducto == item.idProducto ) {
               precio = p.precio;
               nombreProd = p.nombre;
            }
         }
      }
      long subtotal = (long) precio * item.cantidad;
      total += subtotal;
      item.precioUnitario = precio;   // FIX: faltaba guardar el precio real en el item antes de crearOrden
      resp += nombreProd + ";" + std::to_string( item.cantidad ) + ";" +
              std::to_string( precio ) + ";" + std::to_string( subtotal ) + "\n";
   }

   static uint32_t contadorOrden = 1000;
   uint32_t idOrden = contadorOrden++;
   fs.crearOrden( idOrden, items );

   return "OK:" + std::to_string( idOrden ) + "\n" + resp + "TOTAL:" + std::to_string( total ) + "\n";

}


/**
  *  task
  *     Atiende una solicitud del intermediario: lee la linea de comando,
  *     la enruta, responde, y cierra la conexion.
  *
 **/
void task( VSocket * intermediario ) {

   char buffer[ BUFSIZE ] = { 0 };
   intermediario->Read( buffer, BUFSIZE - 1 );
   std::string linea( buffer );

   // quitamos el salto de linea final si vino incluido
   size_t fin = linea.find_first_of( "\r\n" );
   if ( std::string::npos != fin ) linea = linea.substr( 0, fin );

   bitacora.log( "Solicitud del intermediario: " + linea, ServidorProductos );

   std::string respuesta;

   if ( 0 == linea.find( "PEDIR:CAT" ) ) {

      respuesta = ManejarCat();

   } else if ( 0 == linea.find( "PEDIR:PROD:" ) ) {

      respuesta = ManejarProd( linea.substr( strlen( "PEDIR:PROD:" ) ) );

   } else if ( 0 == linea.find( "AGREGAR:" ) ) {

      std::string resto = linea.substr( strlen( "AGREGAR:" ) );
      size_t sep = resto.find( ':' );
      if ( std::string::npos == sep ) {
         respuesta = "ERR:formato invalido, use AGREGAR:idProducto:cantidad\n";
      } else {
         uint32_t idProd = (uint32_t) std::stoul( resto.substr( 0, sep ) );
         uint32_t cant   = (uint32_t) std::stoul( resto.substr( sep + 1 ) );
         respuesta = ManejarAgregar( idProd, cant );
      }

   } else if ( 0 == linea.find( "PEDIR:FACTURA:" ) ) {

      respuesta = ManejarFactura( linea.substr( strlen( "PEDIR:FACTURA:" ) ) );

   } else if ( 0 == linea.find( "SHUTDOWN" ) ) {

      intermediario->Write( "OK\n" );
      intermediario->Close();
      bitacora.log( "Bodega cerrada por solicitud", ServidorProductos );
      delete intermediario;
      exit( 0 );

   } else {

      respuesta = "ERR:comando desconocido\n";

   }

   intermediario->Write( respuesta.c_str() );
   intermediario->Close();
   delete intermediario;

}


int main() {

   std::ifstream pruebaExiste( "bodega_ticamazon.dat" );
   bool archivoEraNuevo = !pruebaExiste.good();
   pruebaExiste.close();

   fsPtr = new FileSystem( "bodega_ticamazon.dat" );

   SembrarCatalogoSiEsNuevo( archivoEraNuevo );

   VSocket * s1 = new Socket( 's' );

   s1->Bind( PORT );
   s1->MarkPassive( 10 );

   std::cout << "[BODEGA] Escuchando (protocolo interno) en el puerto " << PORT << "\n";
   bitacora.log( "Bodega iniciada en el puerto " + std::to_string( PORT ), ServidorProductos );

   for ( ; ; ) {
      VSocket * intermediario = s1->AcceptConnection();
      std::thread worker( task, intermediario );
      worker.detach();
   }

   return 0;

}
