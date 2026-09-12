/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  TicAmazon - Cliente interactivo
  *
  *  Recordatorio del enunciado: HTTP es el protocolo de comunicacion entre
  *  el cliente y el intermediario (aqui, el servidor de productos hace ese
  *  papel directamente). El cliente arma peticiones HTTP GET usando nuestra
  *  propia jerarquia VSocket/Socket, y guarda localmente el carrito hasta
  *  pedir la factura proforma final.
  *
 **/

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <cstring>
#include <cctype>

#include "Socket.h"
#include "Logger.hpp"

#define MAXBUF 4096

/**
  *  UrlEncode
  *     Codifica espacios y caracteres especiales antes de insertarlos en
  *     una ruta HTTP (ej: "Alimentos y bebidas" -> "Alimentos%20y%20bebidas")
  *
 **/
std::string UrlEncode( const std::string & texto ) {
   std::ostringstream out;
   for ( unsigned char c : texto ) {
      if ( isalnum( c ) || c == '-' || c == '_' || c == '.' || c == '~' ) {
         out << c;
      } else {
         out << '%' << std::uppercase << std::hex << (int) c << std::nouppercase << std::dec;
      }
   }
   return out.str();
}


/**
  *  HacerPeticion
  *     Abre una conexion nueva, manda un GET simple, y devuelve el cuerpo
  *     de la respuesta HTTP (sin los encabezados)
  *
 **/
std::string HacerPeticion( const char * host, const char * puerto, const std::string & path, Logger & log ) {

   VSocket * conexion = new Socket( 's' );

   std::string request =
      "GET " + path + " HTTP/1.1\r\n"
      "Host: " + std::string( host ) + "\r\n"
      "Connection: close\r\n\r\n";

   conexion->Connect( host, puerto );
   log.log( request, Cliente );
   conexion->Write( request.c_str() );

   std::string respuesta;
   char buffer[ MAXBUF ];
   int leidos;
   while ( ( leidos = conexion->Read( buffer, MAXBUF - 1 ) ) > 0 ) {
      buffer[ leidos ] = '\0';
      respuesta += buffer;
   }

   delete conexion;

   size_t finEncabezados = respuesta.find( "\r\n\r\n" );
   if ( std::string::npos != finEncabezados ) {
      respuesta = respuesta.substr( finEncabezados + 4 );
   }

   return respuesta;

}


/**
  *  ExtraerEntreEtiquetas
  *     Utilidad minima para mostrar el HTML devuelto por el servidor como
  *     texto legible en la consola (extrae el contenido entre pares de tag)
  *
 **/
std::vector<std::string> ExtraerEntreEtiquetas( const std::string & html, const std::string & tagApertura, const std::string & tagCierre ) {

   std::vector<std::string> resultado;
   size_t pos = 0;

   while ( true ) {
      size_t ini = html.find( tagApertura, pos );
      if ( std::string::npos == ini ) break;
      ini += tagApertura.size();
      size_t fin = html.find( tagCierre, ini );
      if ( std::string::npos == fin ) break;
      resultado.push_back( html.substr( ini, fin - ini ) );
      pos = fin + tagCierre.size();
   }

   return resultado;

}


void MostrarCategorias( const std::string & html ) {

   auto items = ExtraerEntreEtiquetas( html, "<li>", "</li>" );
   std::cout << "\nCategorias disponibles:\n";
   for ( auto & item : items ) {
      // cada <li> trae un <a href="...">Nombre</a>; extraemos el texto visible
      // (entre el cierre del tag de apertura ">" y el inicio del tag de cierre "<")
      size_t inicioTexto = item.find( '>' );
      std::string nombre = item;
      if ( std::string::npos != inicioTexto ) {
         size_t finTexto = item.find( '<', inicioTexto );
         nombre = item.substr( inicioTexto + 1, finTexto - inicioTexto - 1 );
      }
      std::cout << "  - " << nombre << "\n";
   }

}


void MostrarTabla( const std::string & html ) {

   auto celdas = ExtraerEntreEtiquetas( html, "<td>", "</td>" );
   for ( size_t i = 0; i < celdas.size(); i++ ) {
      std::cout << celdas[ i ];
      std::cout << ( ( i % 4 == 3 ) ? "\n" : "\t| " );
   }
   std::cout << std::endl;

   // Tambien mostramos cualquier <p> suelto (totales, mensajes de error, confirmaciones)
   auto parrafos = ExtraerEntreEtiquetas( html, "<p>", "</p>" );
   for ( auto & p : parrafos ) {
      std::string limpio = p;
      // quita un posible <b>/</b> alrededor del total
      size_t b1 = limpio.find( "<b>" );
      if ( std::string::npos != b1 ) limpio.erase( b1, 3 );
      size_t b2 = limpio.find( "</b>" );
      if ( std::string::npos != b2 ) limpio.erase( b2, 4 );
      std::cout << limpio << "\n";
   }

}


int main() {

   const char * host = "127.0.0.1";
   const char * puerto = "8080";

   Logger log( "./bitacora_cliente.log" );

   std::vector<std::pair<uint32_t,uint32_t>> carrito;   // {idProducto, cantidad}

   bool corriendo = true;

   std::cout << "=== TicAmazon - Cliente ===\n";
   std::cout << "1) Listar categorias\n";
   std::cout << "2) Ver productos de una categoria\n";
   std::cout << "3) Agregar producto al carrito\n";
   std::cout << "4) Pedir factura proforma\n";
   std::cout << "5) Salir\n";

   while ( corriendo ) {

      std::cout << "\nComando: ";
      std::string comando;
      std::getline( std::cin, comando );

      if ( "1" == comando ) {

         std::string resp = HacerPeticion( host, puerto, "/categorias", log );
         MostrarCategorias( resp );

      } else if ( "2" == comando ) {

         std::cout << "Nombre de la categoria: ";
         std::string categoria;
         std::getline( std::cin, categoria );

         std::string resp = HacerPeticion( host, puerto, "/productos/" + UrlEncode( categoria ), log );
         std::cout << "\nId\t| Producto\t| Precio\t| Stock\n";
         MostrarTabla( resp );

      } else if ( "3" == comando ) {

         std::cout << "Id de producto: ";
         std::string idStr;
         std::getline( std::cin, idStr );
         std::cout << "Cantidad: ";
         std::string cantStr;
         std::getline( std::cin, cantStr );

         uint32_t id = (uint32_t) std::stoul( idStr );
         uint32_t cant = (uint32_t) std::stoul( cantStr );

         std::string resp = HacerPeticion( host, puerto, "/agregar/" + idStr + "/" + cantStr, log );
         auto parrafos = ExtraerEntreEtiquetas( resp, "<p>", "</p>" );
         for ( auto & p : parrafos ) std::cout << p << "\n";

         if ( !parrafos.empty() && parrafos[ 0 ].find( "OK" ) == 0 ) {
            carrito.push_back( { id, cant } );
         }

      } else if ( "4" == comando ) {

         if ( carrito.empty() ) {
            std::cout << "El carrito esta vacio.\n";
            continue;
         }

         std::ostringstream items;
         for ( size_t i = 0; i < carrito.size(); i++ ) {
            if ( i > 0 ) items << ",";
            items << carrito[ i ].first << ":" << carrito[ i ].second;
         }

         std::string resp = HacerPeticion( host, puerto, "/factura?items=" + items.str(), log );
         std::cout << "\nProducto\t| Cantidad\t| Precio unit.\t| Subtotal\n";
         MostrarTabla( resp );

         carrito.clear();

      } else if ( "5" == comando ) {

         std::cout << "Saliendo...\n";
         log.log( "Cliente finalizo la sesion", Cliente );
         corriendo = false;

      } else {

         std::cout << "Comando invalido.\n";

      }

   }

   return 0;

}
