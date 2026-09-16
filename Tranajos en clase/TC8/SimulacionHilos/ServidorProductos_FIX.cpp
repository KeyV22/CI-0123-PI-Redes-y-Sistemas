/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  TicAmazon - Servidor de productos (real, con sockets)
  *
  *  Decisiones de diseno de esta entrega:
  *    - Modelo de atencion: HILOS (un std::thread por conexion aceptada).
  *      Se eligio sobre fork() porque el servidor necesita compartir una
  *      unica instancia del file system entre todas las solicitudes (varios
  *      clientes viendo/comprando del mismo inventario), y con hilos eso es
  *      memoria compartida directa protegida con un solo mutex, en vez de
  *      tener que sincronizar N procesos separados contra el mismo archivo.
  *    - Modelo del contenedor: el file system propio ya construido
  *      (FileSystem.hpp/.cpp) -- bloques de 256 bytes, superbloque, bitmap,
  *      indice de categorias y de ordenes, tal como fue documentado.
  *    - Salida de datos al cliente: HTML especializado (minimal), para que
  *      la respuesta se pueda ver tanto desde nuestro cliente propio como
  *      desde un navegador comun apuntando directamente a este servidor.
  *
  *  Protocolo expuesto por HTTP (mismo vocabulario ya definido para
  *  intermediario<->productos, aqui expuesto como rutas):
  *
  *     GET /categorias                          -> PEDIR:CAT
  *     GET /productos/<nombreCategoria>         -> PEDIR:PROD
  *     GET /agregar/<idProducto>/<cantidad>      -> AGREGAR
  *     GET /factura?items=id:cant,id:cant,...    -> PEDIR:FACTURA
  *     GET /shutdown                             -> cierra el servidor
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

#include "Socket.h"
#include "FileSystem.hpp"
#include "Logger.hpp"

#define PORT 8080
#define BUFSIZE 2048

static FileSystem * fsPtr = nullptr;
#define fs (*fsPtr)
static std::mutex fsMutex;                 // protege el file system entre hilos concurrentes
static Logger bitacora("./bitacora_productos.log");

// Catalogo de categorias de esta bodega: nombre -> id numerico interno del file system.
// El file system solo indexa por id; este mapa lo mantiene el servidor.
static std::map<std::string, uint8_t> catalogoIds = {
   { "Alimentos",       1 },
   { "Bloques",   2 },
   { "Vehiculos",                 3 }
};

/**
  *  SembrarCatalogoSiEsNuevo
  *     Si la bodega se acaba de crear (archivo nuevo), la llena con un
  *     catalogo de ejemplo. Si el archivo ya existia (persistencia entre
  *     corridas), no vuelve a sembrar para no duplicar productos.
  *
 **/
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

   fs.agregarProducto( 1, "Alimentos", crear( 101, "Lata de refresco", 650, 40 ) );
   fs.agregarProducto( 1, "Alimentos", crear( 102, "Bolsa de papas", 900, 25 ) );
   fs.agregarProducto( 1, "Alimentos", crear( 103, "Barra de chocolate", 750, 30 ) );

   fs.agregarProducto( 2, "Bloques", crear( 201, "Bloque 2x4 rojo", 300, 120 ) );
   fs.agregarProducto( 2, "Bloques", crear( 202, "Bloque 2x2 azul", 250, 95 ) );
   fs.agregarProducto( 2, "Bloques", crear( 203, "Placa base verde", 1200, 15 ) );

   fs.agregarProducto( 3, "Vehiculos", crear( 301, "Rueda pequena", 400, 60 ) );
   fs.agregarProducto( 3, "Vehiculos", crear( 302, "Chasis basico", 1500, 20 ) );
   fs.agregarProducto( 3, "Vehiculos", crear( 303, "Motor de juguete", 2200, 10 ) );

   std::cout << "[SERVIDOR PRODUCTOS] Catalogo de ejemplo sembrado (bodega nueva)\n";

}


// --- utilidades HTTP / HTML ---

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


// --- manejadores de cada ruta (protocolo PEDIR:CAT / PEDIR:PROD / AGREGAR / PEDIR:FACTURA) ---

std::string ManejarCategorias() {

   std::lock_guard<std::mutex> guard( fsMutex );

   std::string filas;
   for ( auto & nombre : fs.listarCategorias() ) {
      filas += "<li><a href=\"/productos/" + nombre + "\">" + nombre + "</a></li>";
   }

   return HtmlPagina( "Categorias disponibles", "<ul>" + filas + "</ul>" );

}

std::string ManejarProductos( const std::string & nombreCategoria ) {

   auto it = catalogoIds.find( nombreCategoria );
   if ( catalogoIds.end() == it ) {
      return HtmlPagina( "Categoria no encontrada", "<p>La categoria \"" + nombreCategoria + "\" no existe.</p>" );
   }

   std::lock_guard<std::mutex> guard( fsMutex );

   std::string filas;
   for ( auto & p : fs.listarProductos( it->second ) ) {
      filas += "<tr><td>" + std::to_string( p.idProducto ) + "</td><td>" + std::string( p.nombre ) +
               "</td><td>" + std::to_string( p.precio ) + "</td><td>" + std::to_string( p.cantidadDisponible ) + "</td></tr>";
   }

   std::string tabla =
      "<table border=\"1\" cellpadding=\"6\"><tr><th>Id</th><th>Producto</th><th>Precio</th><th>Stock</th></tr>"
      + filas + "</table>";

   return HtmlPagina( "Productos de " + nombreCategoria, tabla );

}

std::string ManejarAgregar( uint32_t idProducto, uint32_t cantidad ) {

   std::lock_guard<std::mutex> guard( fsMutex );

   std::string error;
   bool ok = fs.reservarProducto( idProducto, cantidad, &error );

   if ( ok ) {
      return HtmlPagina( "Producto agregado",
         "<p>OK: se reservaron " + std::to_string( cantidad ) + " unidad(es) del producto " +
         std::to_string( idProducto ) + ".</p>" );
   }

   return HtmlPagina( "No se pudo agregar", "<p>ERROR: " + error + "</p>" );

}

std::string ManejarFactura( const std::string & itemsCrudo ) {

   // itemsCrudo viene como "idProducto:cantidad,idProducto:cantidad,..."
   std::vector<ItemOrden> items;
   std::stringstream ss( itemsCrudo );
   std::string par;

   while ( std::getline( ss, par, ',' ) ) {
      size_t sep = par.find( ':' );
      if ( std::string::npos == sep ) continue;
      uint32_t idProd = (uint32_t) std::stoul( par.substr( 0, sep ) );
      uint16_t cant   = (uint16_t) std::stoul( par.substr( sep + 1 ) );
      items.push_back( { idProd, cant, 0 } );          // precioUnitario se completa abajo
   }

   if ( items.empty() ) {
      return HtmlPagina( "Factura vacia", "<p>No se especificaron productos para la factura.</p>" );
   }

   std::lock_guard<std::mutex> guard( fsMutex );

   // Buscamos el precio real de cada producto recorriendo las 3 categorias sembradas
   std::string filas;
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
      filas += "<tr><td>" + nombreProd + "</td><td>" + std::to_string( item.cantidad ) +
               "</td><td>" + std::to_string( precio ) + "</td><td>" + std::to_string( subtotal ) + "</td></tr>";
   }

   static uint32_t contadorOrden = 1000;
   uint32_t idOrden = contadorOrden++;
   fs.crearOrden( idOrden, items );

   std::string tabla =
      "<table border=\"1\" cellpadding=\"6\"><tr><th>Producto</th><th>Cantidad</th><th>Precio unit.</th><th>Subtotal</th></tr>"
      + filas + "</table>"
      + "<p><b>Total: " + std::to_string( total ) + "</b></p>"
      + "<p>Orden proforma #" + std::to_string( idOrden ) + " registrada.</p>";

   return HtmlPagina( "Factura proforma", tabla );

}


/**
  *  task
  *     Atiende una conexion de cliente en su propio hilo: lee la peticion
  *     HTTP, la enruta segun el path, y responde.
  *
 **/
void task( VSocket * cliente ) {

   char buffer[ BUFSIZE ] = { 0 };
   cliente->Read( buffer, BUFSIZE - 1 );
   std::string request( buffer );

   bitacora.log( request, ServidorProductos );

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
         uint32_t idProd = (uint32_t) std::stoul( resto.substr( 0, barra ) );
         uint32_t cant   = (uint32_t) std::stoul( resto.substr( barra + 1 ) );
         respuesta = HttpResponder( 200, "OK", ManejarAgregar( idProd, cant ) );
      }

   } else if ( 0 == path.find( "/factura" ) ) {

      std::string itemsCrudo;
      size_t q = path.find( "?items=" );
      if ( std::string::npos != q ) {
         itemsCrudo = UrlDecode( path.substr( q + strlen( "?items=" ) ) );
      }
      respuesta = HttpResponder( 200, "OK", ManejarFactura( itemsCrudo ) );

   } else if ( 0 == path.find( "/shutdown" ) ) {

      respuesta = HttpResponder( 200, "OK", HtmlPagina( "Cerrando", "<p>Servidor apagandose...</p>" ) );
      cliente->Write( respuesta.c_str() );
      cliente->Close();
      bitacora.log( "Servidor de productos cerrado por solicitud", ServidorProductos );
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

   std::ifstream pruebaExiste( "bodega_ticamazon.dat" );
   bool archivoEraNuevo = !pruebaExiste.good();
   pruebaExiste.close();

   fsPtr = new FileSystem( "bodega_ticamazon.dat" );

   SembrarCatalogoSiEsNuevo( archivoEraNuevo );

   VSocket * s1 = new Socket( 's' );

   s1->Bind( PORT );
   s1->MarkPassive( 10 );

   std::cout << "[SERVIDOR PRODUCTOS] Escuchando en el puerto " << PORT << " (modelo de atencion: hilos)\n";
   bitacora.log( "Servidor de productos iniciado en el puerto " + std::to_string( PORT ), ServidorProductos );

   for ( ; ; ) {
      VSocket * cliente = s1->AcceptConnection();
      std::thread worker( task, cliente );
      worker.detach();
   }

   return 0;

}
