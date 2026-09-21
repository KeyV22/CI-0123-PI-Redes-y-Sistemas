/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *
  *  TicAmazon - Cliente, protocolo mancomunado
  *
 **/

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Socket.h"
#include "Logger.hpp"
#include "Protocolo.hpp"

#define BUFSIZE 4096

static std::string MI_ID = "CLI_04";
static std::string INT_ID = "INT_04";


std::string EnviarMensaje( const std::string & host, const std::string & puerto, const std::string & mensaje, Logger & log ) {

   VSocket * conexion = new Socket( 's' );
   conexion->Connect( host.c_str(), puerto.c_str() );

   std::ostringstream request;
   request << "POST / HTTP/1.1\r\n"
            << "Host: " << host << "\r\n"
            << "Content-Length: " << mensaje.size() << "\r\n"
            << "Connection: close\r\n\r\n"
            << mensaje;

   log.log( mensaje, Cliente );
   conexion->Write( request.str().c_str() );

   std::string respuesta;
   char buffer[ BUFSIZE ];
   int leidos;
   while ( ( leidos = conexion->Read( buffer, BUFSIZE - 1 ) ) > 0 ) {
      buffer[ leidos ] = '\0';
      respuesta += buffer;
   }
   delete conexion;

   size_t finEncabezados = respuesta.find( "\r\n\r\n" );
   std::string cuerpo = ( std::string::npos != finEncabezados ) ? respuesta.substr( finEncabezados + 4 ) : respuesta;

   log.log( cuerpo, Cliente );

   return cuerpo;

}


void MostrarSiEsError( const MensajeV2 & r ) {

   if ( TipoMensaje::ERROR_FORMATO == r.tipo ) {
      std::cout << "  [ERROR 90 - formato invalido] campo=" << ( r.campos.size() > 1 ? r.campos[1] : "?" )
                << " valor=" << ( r.campos.size() > 2 ? r.campos[2] : "?" ) << "\n";
   } else if ( TipoMensaje::ERROR_TAMANO == r.tipo ) {
      std::cout << "  [ERROR 91 - tamano invalido] campo=" << ( r.campos.size() > 1 ? r.campos[1] : "?" ) << "\n";
   } else if ( TipoMensaje::ERROR_COMUNICACION == r.tipo ) {
      std::cout << "  [ERROR 92 - sin respuesta de " << ( r.campos.size() > 0 ? r.campos[0] : "?" ) << "]\n";
   }

}


int main( int argc, char ** argv ) {

   std::string host = ( argc > 1 ) ? argv[ 1 ] : "127.0.0.1";
   std::string puerto = ( argc > 2 ) ? argv[ 2 ] : "8080";
   if ( argc > 3 ) MI_ID = argv[ 3 ];

   Logger log( "./bitacora_cliente_" + MI_ID + ".log" );

   std::cout << "=== TicAmazon - Cliente (" << MI_ID << ") ===\n";
   std::cout << "1) Listar categorias\n"
                "2) Ver productos de una categoria\n"
                "3) Agregar producto al carrito\n"
                "4) Pedir factura proforma\n"
                "5) Salir\n";

   bool corriendo = true;
   while ( corriendo ) {

      std::cout << "\nComando: ";
      std::string comando;
      if ( !std::getline( std::cin, comando ) ) break;

      if ( "1" == comando ) {

         std::string msg = construirMensaje( MI_ID, INT_ID, TipoMensaje::REQUEST_CATEGORY, {} );
         std::string resp = EnviarMensaje( host, puerto, msg, log );
         MensajeV2 r = parsearMensaje( resp );
         MostrarSiEsError( r );
         if ( TipoMensaje::CATEGORY_LIST_INT == r.tipo && !r.campos.empty() ) {
            std::cout << "Categorias disponibles: " << r.campos[ 0 ] << "\n";
         }

      } else if ( "2" == comando ) {

         std::cout << "Categoria: ";
         std::string categoria;
         std::getline( std::cin, categoria );

         std::string msg = construirMensaje( MI_ID, INT_ID, TipoMensaje::REQUEST_PRODUCTS_C, { categoria } );
         std::string resp = EnviarMensaje( host, puerto, msg, log );
         MensajeV2 r = parsearMensaje( resp );
         MostrarSiEsError( r );

         if ( TipoMensaje::PRODUCT_LIST_C == r.tipo && r.campos.size() >= 2 ) {
            std::cout << "Productos (" << r.campos[ 0 ] << "):\n";
            std::stringstream ss( r.campos[ 1 ] );
            std::string item;
            while ( std::getline( ss, item, ';' ) ) {
               std::cout << "  - " << item << "\n";
            }
         } else if ( TipoMensaje::NO_PRODUCTS == r.tipo ) {
            std::cout << ( r.campos.empty() ? "Sin productos" : r.campos[ 0 ] ) << "\n";
         }

      } else if ( "3" == comando ) {

         std::cout << "Nombre del producto: ";
         std::string producto;
         std::getline( std::cin, producto );
         std::cout << "Cantidad: ";
         std::string cantidad;
         std::getline( std::cin, cantidad );

         std::string msg = construirMensaje( MI_ID, INT_ID, TipoMensaje::ADD_TO_CART, { producto, cantidad } );
         std::string resp = EnviarMensaje( host, puerto, msg, log );
         MensajeV2 r = parsearMensaje( resp );
         MostrarSiEsError( r );

         if ( TipoMensaje::CART_OK == r.tipo && r.campos.size() >= 3 ) {
            std::cout << "Agregado: " << r.campos[0] << " x" << r.campos[2] << " (precio " << r.campos[1] << ")\n";
         } else if ( TipoMensaje::CART_NO_STOCK == r.tipo && r.campos.size() >= 3 ) {
            std::cout << "Sin stock suficiente: disponible=" << r.campos[1] << " pedido=" << r.campos[2] << "\n";
         } else if ( TipoMensaje::PRODUCT_NOT_FOUND2 == r.tipo ) {
            std::cout << "Producto no encontrado: " << ( r.campos.empty() ? "" : r.campos[0] ) << "\n";
         }

      } else if ( "4" == comando ) {

         std::string msg = construirMensaje( MI_ID, INT_ID, TipoMensaje::REQUEST_FACTURA, {} );
         std::string resp = EnviarMensaje( host, puerto, msg, log );
         MensajeV2 r = parsearMensaje( resp );
         MostrarSiEsError( r );

         if ( TipoMensaje::FACTURA == r.tipo && r.campos.size() >= 3 ) {
            std::cout << "Factura -- total: " << r.campos[0] << " (" << r.campos[1] << " item(s))\n";
            std::stringstream ss( r.campos[2] );
            std::string item;
            while ( std::getline( ss, item, ';' ) ) {
               std::cout << "  - " << item << "\n";
            }
         } else if ( TipoMensaje::CARRITO_VACIO == r.tipo ) {
            std::cout << ( r.campos.empty() ? "Carrito vacio" : r.campos[0] ) << "\n";
         }

      } else if ( "5" == comando ) {

         std::cout << "Saliendo...\n";
         corriendo = false;

      } else {

         std::cout << "Comando invalido.\n";

      }

   }

   return 0;

}
