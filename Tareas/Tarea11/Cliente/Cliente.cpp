/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *
  *  TicAmazon - Cliente de consola para el Intermediario web
  *
  *  El Cliente.cpp anterior hablaba el protocolo mancomunado directo en el
  *  cuerpo HTTP. Esta version de Intermediario.cpp ya no hace eso: sirve
  *  HTML de verdad para navegar desde un browser (rutas GET /categorias,
  *  /productos/<cat>, /agregar/<prod>/<cant>, /factura). Este programa es
  *  un cliente funcional para ESA version: hace las mismas peticiones GET
  *  que haria un navegador, usando los mismos Socket/SSLSocket del
  *  proyecto (nada de librerias externas de HTTP), y le saca a mano el
  *  contenido util del HTML de respuesta para mostrarlo como un menu de
  *  consola.
  *
  *  Como el Intermediario cierra la conexion despues de cada respuesta
  *  (un hilo por conexion, una peticion por conexion -- ver task() en
  *  Intermediario.cpp), este cliente abre una conexion NUEVA por cada
  *  opcion del menu, igual que haria el navegador con cada click.
  *
  *  Uso:
  *     ./Cliente.out [host] [puerto] [ssl] [idCliente]
  *
  *  Ejemplos:
  *     ./Cliente.out
  *     ./Cliente.out 127.0.0.1 8080
  *     ./Cliente.out 127.0.0.1 8443 ssl
  *     ./Cliente.out 127.0.0.1 8080 "" CLI_09
  *
 **/

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cctype>
#include <limits>

#include "Socket.hpp"
#include "SSLSocket.hpp"

#define BUFSIZE 8192


// ===== utilidades HTTP =====

std::string UrlEncode( const std::string & texto ) {
   std::ostringstream out;
   for ( unsigned char c : texto ) {
      if ( isalnum( c ) || c == '-' || c == '_' || c == '.' || c == '~' ) out << c;
      else out << '%' << std::uppercase << std::hex << (int) c << std::nouppercase << std::dec;
   }
   return out.str();
}

/**
  *  HacerPeticion
  *     Abre una conexion nueva (Socket o SSLSocket segun corresponda) y
  *     manda un GET HTTP/1.1 simple. La respuesta se lee usando el
  *     "Content-Length" del encabezado (como hace un cliente HTTP real),
  *     en vez de leer "hasta que el servidor cierre": con SSLSocket ese
  *     ultimo Read() (el que deberia devolver 0 de EOF limpio) puede
  *     lanzar excepcion en vez de devolver 0 cuando OpenSSL 3.x cierra el
  *     socket TCP sin mandar antes el "close_notify" de TLS -- es un caso
  *     de borde de SSLSocket::Read que nunca se disparaba antes porque el
  *     resto del proyecto solo hace una lectura por conexion (nunca un
  *     "while (Read() > 0)"). Usando Content-Length se evita ese ultimo
  *     Read() innecesario y de paso el cliente queda mas correcto.
  *
 **/
std::string HacerPeticion( const std::string & host, const std::string & puerto, bool usarSSL, const std::string & path ) {

   // Connect() no es virtual en VSocket (igual que en EnviarABodega, en
   // Intermediario.cpp), asi que hay que llamarlo sobre el tipo concreto
   // antes de subir el puntero a VSocket* para el resto del codigo.
   VSocket * conexion;
   if ( usarSSL ) {
      SSLSocket * ssl = new SSLSocket();
      ssl->Connect( host.c_str(), puerto.c_str() );
      conexion = ssl;
   } else {
      Socket * tcp = new Socket( 's' );
      tcp->Connect( host.c_str(), puerto.c_str() );
      conexion = tcp;
   }

   std::string peticion = "GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
   conexion->Write( peticion.c_str() );

   std::string completa;
   char buffer[ BUFSIZE ];
   size_t leidos;

   // Primero se lee hasta tener los encabezados completos (fin "\r\n\r\n").
   size_t finEncabezados;
   while ( std::string::npos == ( finEncabezados = completa.find( "\r\n\r\n" ) ) ) {
      leidos = conexion->Read( buffer, BUFSIZE - 1 );
      if ( 0 == leidos ) break;   // el servidor cerro sin mandar nada util
      completa.append( buffer, leidos );
   }

   // Content-Length dice exactamente cuantos bytes de cuerpo faltan.
   size_t contentLength = 0;
   size_t clPos = completa.find( "Content-Length:" );
   if ( std::string::npos != clPos ) {
      try { contentLength = (size_t) std::stoul( completa.substr( clPos + strlen( "Content-Length:" ) ) ); }
      catch ( ... ) { contentLength = 0; }
   }

   size_t bytesCuerpoYaLeidos = ( std::string::npos != finEncabezados ) ? ( completa.size() - ( finEncabezados + 4 ) ) : 0;
   while ( bytesCuerpoYaLeidos < contentLength ) {
      leidos = conexion->Read( buffer, BUFSIZE - 1 );
      if ( 0 == leidos ) break;   // el servidor cerro antes de lo esperado
      completa.append( buffer, leidos );
      bytesCuerpoYaLeidos += leidos;
   }

   delete conexion;

   std::string html = ( std::string::npos == finEncabezados ) ? completa : completa.substr( finEncabezados + 4 );

   // Se descarta todo lo que no sea el <body>...</body> (el <head> trae el
   // <title> y el <style>, que no sirven para un cliente de consola y si no
   // se quitan se mezclan con el texto util al quitar las etiquetas).
   size_t inicioBody = html.find( "<body>" );
   size_t finBody = html.find( "</body>" );
   if ( std::string::npos != inicioBody && std::string::npos != finBody ) {
      return html.substr( inicioBody + strlen( "<body>" ), finBody - inicioBody - strlen( "<body>" ) );
   }
   return html;

}


// ===== utilidades para extraer contenido util del HTML de respuesta =====

std::string QuitarEtiquetas( const std::string & html ) {
   std::string resultado;
   bool dentroDeEtiqueta = false;
   for ( char c : html ) {
      if ( '<' == c ) { dentroDeEtiqueta = true; continue; }
      if ( '>' == c ) { dentroDeEtiqueta = false; continue; }
      if ( !dentroDeEtiqueta ) resultado += c;
   }
   return resultado;
}

// Devuelve el contenido de cada bloque <etiqueta ...> ... </etiqueta> (sin anidar).
// Cuidado: "<b" tambien es prefijo de "<body>", asi que no basta con buscar el
// texto "<etiqueta" -- hay que comprobar que justo despues del nombre viene
// '>', un espacio, o '/' (fin de una etiqueta autocerrada), y no otra letra.
std::vector<std::string> ExtraerBloques( const std::string & html, const std::string & etiqueta ) {
   std::vector<std::string> bloques;
   std::string apertura = "<" + etiqueta;
   std::string cierre = "</" + etiqueta + ">";

   size_t pos = 0;
   while ( true ) {
      size_t inicio = html.find( apertura, pos );
      if ( std::string::npos == inicio ) break;

      char siguiente = ( inicio + apertura.size() < html.size() ) ? html[ inicio + apertura.size() ] : '\0';
      if ( '>' != siguiente && ' ' != siguiente && '\t' != siguiente && '/' != siguiente ) {
         // era otra etiqueta que empieza igual (ej. <body> al buscar "b")
         pos = inicio + apertura.size();
         continue;
      }

      size_t finApertura = html.find( '>', inicio );
      if ( std::string::npos == finApertura ) break;
      size_t fin = html.find( cierre, finApertura );
      if ( std::string::npos == fin ) break;
      bloques.push_back( html.substr( finApertura + 1, fin - finApertura - 1 ) );
      pos = fin + cierre.size();
   }
   return bloques;
}

std::string Recortar( const std::string & texto ) {
   size_t inicio = texto.find_first_not_of( " \t\r\n" );
   if ( std::string::npos == inicio ) return "";
   size_t fin = texto.find_last_not_of( " \t\r\n" );
   return texto.substr( inicio, fin - inicio + 1 );
}


// ===== paginas =====

void MostrarPaginaSimple( const std::string & html ) {
   // titulo (dentro de <h2>) + resto del cuerpo sin etiquetas, para paginas
   // de mensaje (agregar, factura vacia, errores, etc.)
   auto titulos = ExtraerBloques( html, "h2" );
   if ( !titulos.empty() ) std::cout << "\n=== " << titulos[ 0 ] << " ===\n";

   std::string sinEtiquetas = QuitarEtiquetas( html );
   // quita lo que ya se mostro como titulo y deja el resto legible
   if ( !titulos.empty() ) {
      size_t pos = sinEtiquetas.find( titulos[ 0 ] );
      if ( std::string::npos != pos ) sinEtiquetas.erase( pos, titulos[ 0 ].size() );
   }
   std::cout << Recortar( sinEtiquetas ) << "\n";
}

std::vector<std::string> MostrarCategorias( const std::string & html ) {
   auto items = ExtraerBloques( html, "li" );
   std::vector<std::string> categorias;

   if ( items.empty() ) {
      MostrarPaginaSimple( html );
      return categorias;
   }

   std::cout << "\n=== Categorias disponibles ===\n";
   int n = 1;
   for ( auto & item : items ) {
      std::string nombre = Recortar( QuitarEtiquetas( item ) );
      if ( nombre.empty() ) continue;   // el <li> del link "Inicio", etc.
      categorias.push_back( nombre );
      std::cout << "  " << n++ << ") " << nombre << "\n";
   }
   return categorias;
}

struct FilaProducto { std::string nombre, precio, stock; };

std::vector<FilaProducto> MostrarProductos( const std::string & html ) {
   auto filas = ExtraerBloques( html, "tr" );
   std::vector<FilaProducto> productos;

   if ( filas.size() <= 1 ) {   // solo el encabezado (o ninguna tabla)
      MostrarPaginaSimple( html );
      return productos;
   }

   std::cout << "\n=== Productos ===\n";
   int n = 1;
   for ( size_t i = 1; i < filas.size(); i++ ) {   // se salta la fila 0 (encabezado <th>)
      auto celdas = ExtraerBloques( filas[ i ], "td" );
      if ( celdas.size() < 3 ) continue;
      FilaProducto p;
      p.nombre = Recortar( QuitarEtiquetas( celdas[ 0 ] ) );
      p.precio = Recortar( QuitarEtiquetas( celdas[ 1 ] ) );
      p.stock  = Recortar( QuitarEtiquetas( celdas[ 2 ] ) );
      productos.push_back( p );
      std::cout << "  " << n++ << ") " << p.nombre << " - precio: " << p.precio << " - stock: " << p.stock << "\n";
   }
   return productos;
}

void MostrarFactura( const std::string & html ) {
   auto filas = ExtraerBloques( html, "tr" );

   if ( filas.size() <= 1 ) {
      MostrarPaginaSimple( html );
      return;
   }

   std::cout << "\n=== Factura proforma ===\n";
   for ( size_t i = 1; i < filas.size(); i++ ) {
      auto celdas = ExtraerBloques( filas[ i ], "td" );
      if ( celdas.size() < 4 ) continue;
      std::cout << "  " << Recortar( QuitarEtiquetas( celdas[ 0 ] ) )
                 << " x" << Recortar( QuitarEtiquetas( celdas[ 1 ] ) )
                 << " -> " << Recortar( QuitarEtiquetas( celdas[ 3 ] ) ) << "\n";
   }

   auto negritas = ExtraerBloques( html, "b" );
   for ( auto & b : negritas ) {
      if ( 0 == b.find( "Total" ) ) std::cout << "  " << b << "\n";
   }
}


int LeerOpcion( const std::string & mensaje, int minimo, int maximo ) {
   while ( true ) {
      std::cout << mensaje;
      std::string linea;
      if ( !std::getline( std::cin, linea ) ) return -1;
      try {
         int valor = std::stoi( linea );
         if ( valor >= minimo && valor <= maximo ) return valor;
      } catch ( ... ) { }
      std::cout << "Opcion invalida.\n";
   }
}


int main( int argc, char ** argv ) {

   std::string host      = ( argc > 1 ) ? argv[ 1 ] : "127.0.0.1";
   std::string puerto     = ( argc > 2 ) ? argv[ 2 ] : "8080";
   bool usarSSL           = ( argc > 3 && 0 == strcmp( argv[ 3 ], "ssl" ) );
   std::string idCliente  = ( argc > 4 ) ? argv[ 4 ] : "CLI_04";

   std::cout << "===== Cliente TicAmazon =====\n";
   std::cout << "Servidor: " << ( usarSSL ? "https" : "http" ) << "://" << host << ":" << puerto << "\n";
   std::cout << "Cliente: " << idCliente << "\n";

   std::string ultimaCategoria;

   while ( true ) {

      std::cout << "\n--- Menu ---\n"
                << "1) Ver categorias\n"
                << "2) Ver productos de una categoria\n"
                << "3) Agregar producto al carrito\n"
                << "4) Ver factura\n"
                << "5) Salir\n";

      int opcion = LeerOpcion( "Opcion: ", 1, 5 );
      if ( -1 == opcion || 5 == opcion ) break;

      try {

         if ( 1 == opcion ) {

            std::string html = HacerPeticion( host, puerto, usarSSL, "/categorias?cli=" + UrlEncode( idCliente ) );
            MostrarCategorias( html );

         } else if ( 2 == opcion ) {

            std::cout << "Nombre de la categoria: ";
            std::string categoria;
            std::getline( std::cin, categoria );
            if ( categoria.empty() ) continue;

            std::string html = HacerPeticion( host, puerto, usarSSL, "/productos/" + UrlEncode( categoria ) + "?cli=" + UrlEncode( idCliente ) );
            MostrarProductos( html );
            ultimaCategoria = categoria;

         } else if ( 3 == opcion ) {

            std::cout << "Nombre del producto: ";
            std::string producto;
            std::getline( std::cin, producto );
            if ( producto.empty() ) continue;

            std::cout << "Cantidad: ";
            std::string cantidad;
            std::getline( std::cin, cantidad );
            if ( cantidad.empty() ) continue;

            std::string html = HacerPeticion( host, puerto, usarSSL,
               "/agregar/" + UrlEncode( producto ) + "/" + UrlEncode( cantidad ) + "?cli=" + UrlEncode( idCliente ) );
            MostrarPaginaSimple( html );

         } else if ( 4 == opcion ) {

            std::string html = HacerPeticion( host, puerto, usarSSL, "/factura?cli=" + UrlEncode( idCliente ) );
            MostrarFactura( html );

         }

      } catch ( std::exception & e ) {
         std::cout << "Error de comunicacion con el servidor: " << e.what() << "\n";
         std::cout << "(revisa que el Intermediario este corriendo en " << host << ":" << puerto << ")\n";
      }

   }

   std::cout << "Adios.\n";
   return 0;

}
