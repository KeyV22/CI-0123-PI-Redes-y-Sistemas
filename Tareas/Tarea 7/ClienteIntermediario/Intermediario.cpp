/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  TicAmazon - Intermediario
  *
  *  Este es el proceso que el cliente conoce: habla HTTP con el cliente
  *  (mismas rutas que antes), y por cada solicitud abre una conexion nueva
  *  hacia el servidor de productos (bodega) usando el protocolo propio ya
  *  definido (PEDIR:CAT, PEDIR:PROD, AGREGAR, PEDIR:FACTURA), y traduce la
  *  respuesta a HTML para el cliente.
  *
  *     Cliente  <--HTTP-->  Intermediario  <--protocolo propio-->  ServidorProductos (bodega)
  *
  *  Modelo de atencion: hilos (un hilo por conexion de cliente aceptada).
  *
 **/

#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <cstring>
#include <cstdlib>

#include "Socket.h"
#include "Logger.hpp"

#define PORT_CLIENTE   8080
#define BODEGA_HOST    "127.0.0.1"
#define BODEGA_PUERTO  "9090"
#define BUFSIZE        4096

static Logger bitacora("./bitacora_intermediario.log");


/**
  *  ConsultarBodega
  *     Abre una conexion nueva hacia la bodega, manda la linea de comando
  *     del protocolo propio, y devuelve la respuesta completa (texto).
  *
 **/
std::string ConsultarBodega( const std::string & comando ) {

   VSocket * bodega = new Socket( 's' );
   bodega->Connect( BODEGA_HOST, BODEGA_PUERTO );

   std::string linea = comando + "\n";
   bodega->Write( linea.c_str() );

   std::string respuesta;
   char buffer[ BUFSIZE ];
   int leidos;
   while ( ( leidos = bodega->Read( buffer, BUFSIZE - 1 ) ) > 0 ) {
      buffer[ leidos ] = '\0';
      respuesta += buffer;
   }

   delete bodega;

   return respuesta;

}


// --- utilidades HTTP / HTML (mismas que la version anterior) ---

std::string UrlDecode( const std::string & texto ) {
   std::string resultado;
   for ( size_t i = 0; i < texto.size(); i++ ) {
      if ( '%' == texto[ i ] && i + 2 < texto.size() ) {
         int valor = std::stoi( texto.substr( i + 1, 2 ), nullptr, 16 );
         resultado += (char) valor;
         i += 2;
      } else if ( '+' == texto[ i ] ) {
         resultado += ' ';
      } else {
         resultado += texto[ i ];
      }
   }
   return resultado;
}

std::string HtmlPagina( const std::string & titulo, const std::string & cuerpo ) {
   std::string html;
   html += "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";
   html += "<title>TicAmazon - " + titulo + "</title></head><body>";
   html += "<h2>" + titulo + "</h2>";
   html += cuerpo;
   html += "</body></html>";
   return html;
}

std::string HttpResponder( int codigo, const std::string & textoCodigo, const std::string & html ) {
   std::ostringstream resp;
   resp << "HTTP/1.1 " << codigo << " " << textoCodigo << "\r\n"
        << "Content-Type: text/html; charset=utf-8\r\n"
        << "Content-Length: " << html.size() << "\r\n"
        << "Connection: close\r\n\r\n"
        << html;
   return resp.str();
}

// separa una respuesta del protocolo propio en lineas
std::vector<std::string> Lineas( const std::string & texto ) {
   std::vector<std::string> resultado;
   std::stringstream ss( texto );
   std::string linea;
   while ( std::getline( ss, linea ) ) {
      if ( !linea.empty() && '\r' == linea.back() ) linea.pop_back();
      resultado.push_back( linea );
   }
   return resultado;
}


// --- traduccion de cada ruta HTTP al protocolo propio contra la bodega ---

std::string ManejarCategorias() {

   auto lineas = Lineas( ConsultarBodega( "PEDIR:CAT" ) );

   std::string filas;
   for ( size_t i = 1; i < lineas.size(); i++ ) {       // linea 0 es "OK"
      filas += "<li><a href=\"/productos/" + lineas[ i ] + "\">" + lineas[ i ] + "</a></li>";
   }

   return HtmlPagina( "Categorias disponibles", "<ul>" + filas + "</ul>" );

}

std::string ManejarProductos( const std::string & nombreCategoria ) {

   auto lineas = Lineas( ConsultarBodega( "PEDIR:PROD:" + nombreCategoria ) );

   if ( lineas.empty() || 0 == lineas[ 0 ].find( "ERR:" ) ) {
      std::string msg = lineas.empty() ? "Sin respuesta de la bodega" : lineas[ 0 ].substr( 4 );
      return HtmlPagina( "Categoria no encontrada", "<p>" + msg + "</p>" );
   }

   std::string filas;
   for ( size_t i = 1; i < lineas.size(); i++ ) {
      std::stringstream campo( lineas[ i ] );
      std::string id, nombre, precio, stock;
      std::getline( campo, id, ';' );
      std::getline( campo, nombre, ';' );
      std::getline( campo, precio, ';' );
      std::getline( campo, stock, ';' );
      filas += "<tr><td>" + id + "</td><td>" + nombre + "</td><td>" + precio + "</td><td>" + stock + "</td></tr>";
   }

   std::string tabla =
      "<table border=\"1\" cellpadding=\"6\"><tr><th>Id</th><th>Producto</th><th>Precio</th><th>Stock</th></tr>"
      + filas + "</table>";

   return HtmlPagina( "Productos de " + nombreCategoria, tabla );

}

std::string ManejarAgregar( const std::string & idProducto, const std::string & cantidad ) {

   auto lineas = Lineas( ConsultarBodega( "AGREGAR:" + idProducto + ":" + cantidad ) );
   std::string primera = lineas.empty() ? "ERR:sin respuesta" : lineas[ 0 ];

   if ( 0 == primera.find( "OK" ) ) {
      return HtmlPagina( "Producto agregado",
         "<p>OK: se reservaron " + cantidad + " unidad(es) del producto " + idProducto + ".</p>" );
   }

   return HtmlPagina( "No se pudo agregar", "<p>ERROR: " + primera.substr( 4 ) + "</p>" );

}

std::string ManejarFactura( const std::string & itemsCrudo ) {

   auto lineas = Lineas( ConsultarBodega( "PEDIR:FACTURA:" + itemsCrudo ) );

   if ( lineas.empty() || 0 == lineas[ 0 ].find( "ERR:" ) ) {
      std::string msg = lineas.empty() ? "Sin respuesta de la bodega" : lineas[ 0 ].substr( 4 );
      return HtmlPagina( "Factura vacia", "<p>" + msg + "</p>" );
   }

   std::string idOrden = lineas[ 0 ].substr( lineas[ 0 ].find( ':' ) + 1 );

   std::string filas;
   std::string total;
   for ( size_t i = 1; i < lineas.size(); i++ ) {
      if ( 0 == lineas[ i ].find( "TOTAL:" ) ) {
         total = lineas[ i ].substr( strlen( "TOTAL:" ) );
         continue;
      }
      std::stringstream campo( lineas[ i ] );
      std::string nombre, cant, precio, subtotal;
      std::getline( campo, nombre, ';' );
      std::getline( campo, cant, ';' );
      std::getline( campo, precio, ';' );
      std::getline( campo, subtotal, ';' );
      filas += "<tr><td>" + nombre + "</td><td>" + cant + "</td><td>" + precio + "</td><td>" + subtotal + "</td></tr>";
   }

   std::string tabla =
      "<table border=\"1\" cellpadding=\"6\"><tr><th>Producto</th><th>Cantidad</th><th>Precio unit.</th><th>Subtotal</th></tr>"
      + filas + "</table>"
      + "<p><b>Total: " + total + "</b></p>"
      + "<p>Orden proforma #" + idOrden + " registrada.</p>";

   return HtmlPagina( "Factura proforma", tabla );

}


void task( VSocket * cliente ) {

   char buffer[ BUFSIZE ] = { 0 };
   cliente->Read( buffer, BUFSIZE - 1 );
   std::string request( buffer );

   bitacora.log( request, Intermediario );

   std::string respuesta;

   size_t inicioPath = request.find( ' ' ) + 1;
   size_t finPath = request.find( ' ', inicioPath );
   std::string path = ( std::string::npos != finPath ) ? request.substr( inicioPath, finPath - inicioPath ) : "/";

   if ( 0 == path.find( "/categorias" ) ) {

      respuesta = HttpResponder( 200, "OK", ManejarCategorias() );

   } else if ( 0 == path.find( "/productos/" ) ) {

      std::string nombreCategoria = UrlDecode( path.substr( strlen( "/productos/" ) ) );
      respuesta = HttpResponder( 200, "OK", ManejarProductos( nombreCategoria ) );

   } else if ( 0 == path.find( "/agregar/" ) ) {

      std::string resto = path.substr( strlen( "/agregar/" ) );
      size_t barra = resto.find( '/' );
      if ( std::string::npos == barra ) {
         respuesta = HttpResponder( 400, "Bad Request", HtmlPagina( "Peticion invalida", "<p>Uso: /agregar/idProducto/cantidad</p>" ) );
      } else {
         respuesta = HttpResponder( 200, "OK", ManejarAgregar( resto.substr( 0, barra ), resto.substr( barra + 1 ) ) );
      }

   } else if ( 0 == path.find( "/factura" ) ) {

      std::string itemsCrudo;
      size_t q = path.find( "?items=" );
      if ( std::string::npos != q ) {
         itemsCrudo = UrlDecode( path.substr( q + strlen( "?items=" ) ) );
      }
      respuesta = HttpResponder( 200, "OK", ManejarFactura( itemsCrudo ) );

   } else if ( 0 == path.find( "/shutdown" ) ) {

      respuesta = HttpResponder( 200, "OK", HtmlPagina( "Cerrando", "<p>Intermediario apagandose...</p>" ) );
      cliente->Write( respuesta.c_str() );
      cliente->Close();
      bitacora.log( "Intermediario cerrado por solicitud", Intermediario );
      delete cliente;
      exit( 0 );

   } else {

      respuesta = HttpResponder( 404, "Not Found", HtmlPagina( "No encontrado", "<p>Ruta invalida.</p>" ) );

   }

   cliente->Write( respuesta.c_str() );
   cliente->Close();
   delete cliente;

}


int main() {

   VSocket * s1 = new Socket( 's' );

   s1->Bind( PORT_CLIENTE );
   s1->MarkPassive( 10 );

   std::cout << "[INTERMEDIARIO] Escuchando clientes HTTP en el puerto " << PORT_CLIENTE << "\n";
   std::cout << "[INTERMEDIARIO] Hablando con la bodega en " << BODEGA_HOST << ":" << BODEGA_PUERTO << "\n";
   bitacora.log( "Intermediario iniciado en el puerto " + std::to_string( PORT_CLIENTE ), Intermediario );

   for ( ; ; ) {
      VSocket * cliente = s1->AcceptConnection();
      std::thread worker( task, cliente );
      worker.detach();
   }

   return 0;

}
