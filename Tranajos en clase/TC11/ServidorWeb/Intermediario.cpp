/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *
  *  TicAmazon - Intermediario (version pagina web / HTML)
  *
  *  Diferencia con la version anterior (Intermediario.cpp del protocolo
  *  mancomunado "puro"): ahi el cuerpo HTTP llevaba el mensaje del
  *  protocolo mancomunado tal cual (texto plano, para un cliente
  *  programado). Aca en cambio:
  *
  *    - Cliente <-> Intermediario: HTTP con RUTAS (GET), respondiendo HTML
  *      de verdad -- se puede navegar directo desde un browser.
  *    - Intermediario <-> Bodega: SIGUE hablando el protocolo mancomunado
  *      (PEDIR igual que antes, ORIGEN|DESTINO/TIPO/campos), porque esa
  *      capa es maquina-a-maquina y no necesita ser HTML.
  *
  *  Modelo de atencion: hilos (un std::thread por conexion aceptada).
  *
  *  Como HTTP no mantiene sesion, cada link generado en el HTML arrastra
  *  el id del cliente en el query string (?cli=CLI_04) para poder seguir
  *  su carrito entre paginas sin cookies.
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
#include <chrono>
#include <iomanip>
#include <cctype>

#include "Socket.hpp"
#include "SSLSocket.hpp"
#include "Logger.hpp"
#include "Protocolo.hpp"
#include "Descubrimiento.hpp"

#define BUFSIZE 4096
#define REINTENTOS_MAX 2

static std::string MI_ID = "INT_04";
static int PUERTO_HTTP = 8080;
static int PUERTO_MULTICAST = 5000;

static Logger * bitacora = nullptr;
static Descubrimiento * descubrimiento = nullptr;
static VSocket * s1Global = nullptr;

struct ItemCarrito { std::string nombre; std::string precio; int cantidad; };
static std::map<std::string, std::vector<ItemCarrito>> carritos;   // cli -> items
static std::mutex carritosMutex;

static std::map<std::string, InfoPeer> productoEnBodega;   // nombre de producto -> bodega que lo tiene (cache)
static std::mutex cacheMutex;


// ===== comunicacion con la(s) bodega(s), protocolo mancomunado (sin cambios respecto a la version anterior) =====

std::string EnviarABodega( const InfoPeer & bodega, const std::string & mensaje ) {

   Socket * conexion = new Socket( 's' );
   conexion->Connect( bodega.ip.c_str(), std::to_string( bodega.puerto ).c_str() );

   conexion->Write( mensaje.c_str() );

   char buffer[ BUFSIZE ];
   size_t leidos = conexion->Read( buffer, BUFSIZE - 1 );
   buffer[ leidos ] = '\0';

   delete conexion;

   return std::string( buffer );

}


// ===== utilidades HTML / HTTP =====

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

std::string UrlEncode( const std::string & texto ) {
   std::ostringstream out;
   for ( unsigned char c : texto ) {
      if ( isalnum( c ) || c == '-' || c == '_' || c == '.' || c == '~' ) out << c;
      else out << '%' << std::uppercase << std::hex << (int) c << std::nouppercase << std::dec;
   }
   return out.str();
}

std::string HtmlPagina( const std::string & titulo, const std::string & cuerpo ) {
   std::string html;
   html += "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";
   html += "<title>TicAmazon - " + titulo + "</title>";
   html += "<style>body{font-family:Arial,sans-serif;margin:2em;} table{border-collapse:collapse;} "
           "td,th{border:1px solid #ccc;padding:6px 10px;} a{color:#0645AD;} .err{color:#b00;font-weight:bold;}</style>";
   html += "</head><body>";
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


// ===== resolucion de peticiones contra la(s) bodega(s), reusando el protocolo mancomunado =====

bool ConsultarTodasLasBodegas( const std::string & mensaje, std::string * respuestaOut, InfoPeer * bodegaQueRespondioOut,
                                bool (*EsUtil)( const MensajeV2 & ) ) {

   auto bodegas = descubrimiento->BuscarTodos( "SERV" );

   for ( auto & bodega : bodegas ) {
      for ( int intento = 0; intento < REINTENTOS_MAX; intento++ ) {
         try {
            std::string resp = EnviarABodega( bodega, mensaje );
            MensajeV2 m = parsearMensaje( resp );
            if ( nullptr == EsUtil || EsUtil( m ) ) {
               *respuestaOut = resp;
               if ( nullptr != bodegaQueRespondioOut ) *bodegaQueRespondioOut = bodega;
               return true;
            }
            *respuestaOut = resp;
            if ( nullptr != bodegaQueRespondioOut ) *bodegaQueRespondioOut = bodega;
            break;
         } catch ( ... ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            continue;
         }
      }
   }
   return false;
}


// ===== paginas HTML de cada seccion =====

std::string PaginaInicio( const std::string & cli ) {
   std::string cuerpo =
      "<p>Cliente actual: <b>" + cli + "</b></p>"
      "<ul>"
      "<li><a href=\"/categorias?cli=" + UrlEncode( cli ) + "\">Ver categorias</a></li>"
      "<li><a href=\"/factura?cli=" + UrlEncode( cli ) + "\">Ver factura / cerrar pedido</a></li>"
      "</ul>";
   return HtmlPagina( "TicAmazon", cuerpo );
}

std::string PaginaCategorias( const std::string & cli ) {

   std::vector<std::string> categoriasUnicas;
   auto bodegas = descubrimiento->BuscarTodos( "SERV" );
   bool algunaRespondio = false;

   for ( auto & bodega : bodegas ) {
      try {
         std::string resp = EnviarABodega( bodega, construirMensaje( MI_ID, "SERV", TipoMensaje::REQUEST_CATEGORY, {} ) );
         MensajeV2 r = parsearMensaje( resp );
         if ( TipoMensaje::CATEGORY_LIST_INT != r.tipo || r.campos.empty() ) continue;
         algunaRespondio = true;
         std::stringstream ss( r.campos[ 0 ] );
         std::string cat;
         while ( std::getline( ss, cat, ',' ) ) {
            if ( cat.empty() ) continue;
            bool yaEsta = false;
            for ( auto & c : categoriasUnicas ) if ( c == cat ) { yaEsta = true; break; }
            if ( !yaEsta ) categoriasUnicas.push_back( cat );
         }
      } catch ( ... ) { continue; }
   }

   if ( !algunaRespondio ) {
      return HtmlPagina( "Categorias", "<p class=\"err\">No se pudo contactar ninguna bodega (error de comunicacion).</p>" );
   }

   std::string filas;
   for ( auto & cat : categoriasUnicas ) {
      filas += "<li><a href=\"/productos/" + UrlEncode( cat ) + "?cli=" + UrlEncode( cli ) + "\">" + cat + "</a></li>";
   }

   return HtmlPagina( "Categorias disponibles", "<ul>" + filas + "</ul><p><a href=\"/?cli=" + UrlEncode( cli ) + "\">Inicio</a></p>" );

}

std::string PaginaProductos( const std::string & categoria, const std::string & cli ) {

   std::string resp;
   InfoPeer bodegaConLaCategoria;
   bool ok = ConsultarTodasLasBodegas(
      construirMensaje( MI_ID, "SERV", TipoMensaje::REQUEST_PRODUCTS_B, { categoria } ),
      &resp, &bodegaConLaCategoria,
      []( const MensajeV2 & r ) { return TipoMensaje::PRODUCT_LIST_B == r.tipo && !r.campos.empty() && r.campos[ 0 ] != "0"; }
   );

   if ( !ok ) {
      return HtmlPagina( "Sin respuesta", "<p class=\"err\">No se pudo contactar ninguna bodega.</p>" );
   }

   MensajeV2 rBodega = parsearMensaje( resp );

   if ( rBodega.campos.empty() || "0" == rBodega.campos[ 0 ] ) {
      return HtmlPagina( "Sin productos", "<p>No hay productos disponibles en la categoria \"" + categoria + "\".</p>" );
   }

   std::string filas;
   if ( rBodega.campos.size() >= 2 ) {
      std::stringstream ss( rBodega.campos[ 1 ] );
      std::string item;
      while ( std::getline( ss, item, ';' ) ) {
         std::stringstream campo( item );
         std::string nombre, precio, stock;
         std::getline( campo, nombre, ',' );
         std::getline( campo, precio, ',' );
         std::getline( campo, stock, ',' );

         { std::lock_guard<std::mutex> guard( cacheMutex ); productoEnBodega[ nombre ] = bodegaConLaCategoria; }

         filas += "<tr><td>" + nombre + "</td><td>" + precio + "</td><td>" + stock + "</td>"
                  "<td><a href=\"/agregar/" + UrlEncode( nombre ) + "/1?cli=" + UrlEncode( cli ) + "\">agregar 1</a></td></tr>";
      }
   }

   std::string tabla =
      "<table><tr><th>Producto</th><th>Precio</th><th>Stock</th><th></th></tr>" + filas + "</table>"
      "<p><a href=\"/categorias?cli=" + UrlEncode( cli ) + "\">Volver a categorias</a></p>";

   return HtmlPagina( "Productos de " + categoria, tabla );

}

std::string PaginaAgregar( const std::string & producto, const std::string & cantidadStr, const std::string & cli ) {

   InfoPeer bodega;
   {
      std::lock_guard<std::mutex> guard( cacheMutex );
      auto it = productoEnBodega.find( producto );
      if ( productoEnBodega.end() == it ) {
         return HtmlPagina( "No encontrado", "<p class=\"err\">Primero visita la categoria de \"" + producto + "\".</p>" );
      }
      bodega = it->second;
   }

   std::string respDetalle;
   try {
      respDetalle = EnviarABodega( bodega, construirMensaje( MI_ID, "SERV", TipoMensaje::REQUEST_DETAIL_B, { producto } ) );
   } catch ( ... ) {
      return HtmlPagina( "Error", "<p class=\"err\">La bodega no respondio.</p>" );
   }

   MensajeV2 detalle = parsearMensaje( respDetalle );
   if ( TipoMensaje::PRODUCT_NOT_FOUND == detalle.tipo ) {
      return HtmlPagina( "No encontrado", "<p class=\"err\">Producto \"" + producto + "\" no existe.</p>" );
   }

   std::string precio = detalle.campos.size() > 1 ? detalle.campos[ 1 ] : "0";
   int stock = detalle.campos.size() > 2 ? std::stoi( detalle.campos[ 2 ] ) : 0;
   int cantidadPedida = std::stoi( cantidadStr );

   if ( cantidadPedida > stock ) {
      return HtmlPagina( "Sin stock", "<p class=\"err\">Solo hay " + std::to_string( stock ) + " unidad(es) de \"" + producto + "\".</p>" );
   }

   std::string respReserva;
   try {
      respReserva = EnviarABodega( bodega, construirMensaje( MI_ID, "SERV", TipoMensaje::RESERVE_STOCK, { producto, cantidadStr } ) );
   } catch ( ... ) {
      return HtmlPagina( "Error", "<p class=\"err\">No se pudo reservar el producto.</p>" );
   }

   {
      std::lock_guard<std::mutex> guard( carritosMutex );
      carritos[ cli ].push_back( { producto, precio, cantidadPedida } );
   }

   std::string cuerpo =
      "<p>Agregado: <b>" + producto + "</b> x" + cantidadStr + " (precio " + precio + " c/u)</p>"
      "<p><a href=\"/categorias?cli=" + UrlEncode( cli ) + "\">Seguir viendo categorias</a> | "
      "<a href=\"/factura?cli=" + UrlEncode( cli ) + "\">Ver factura</a></p>";

   return HtmlPagina( "Producto agregado", cuerpo );

}

std::string PaginaFactura( const std::string & cli ) {

   std::vector<ItemCarrito> items;
   {
      std::lock_guard<std::mutex> guard( carritosMutex );
      auto it = carritos.find( cli );
      if ( carritos.end() != it ) { items = it->second; carritos.erase( it ); }
   }

   if ( items.empty() ) {
      return HtmlPagina( "Carrito vacio", "<p>El carrito esta vacio.</p><p><a href=\"/categorias?cli=" + UrlEncode( cli ) + "\">Ver categorias</a></p>" );
   }

   std::string filas;
   double total = 0;
   for ( auto & item : items ) {
      double subtotal = std::stod( item.precio ) * item.cantidad;
      total += subtotal;
      std::ostringstream sub; sub << std::fixed << std::setprecision( 2 ) << subtotal;
      filas += "<tr><td>" + item.nombre + "</td><td>" + std::to_string( item.cantidad ) + "</td><td>" + item.precio + "</td><td>" + sub.str() + "</td></tr>";
   }

   std::ostringstream totalStr; totalStr << std::fixed << std::setprecision( 2 ) << total;

   std::string cuerpo =
      "<table><tr><th>Producto</th><th>Cantidad</th><th>Precio</th><th>Subtotal</th></tr>" + filas + "</table>"
      "<p><b>Total: " + totalStr.str() + "</b></p>"
      "<p><a href=\"/?cli=" + UrlEncode( cli ) + "\">Inicio</a></p>";

   return HtmlPagina( "Factura proforma", cuerpo );

}


// ===== ruteo HTTP =====

std::string ObtenerParametro( const std::string & query, const std::string & clave, const std::string & porDefecto ) {
   size_t pos = query.find( clave + "=" );
   if ( std::string::npos == pos ) return porDefecto;
   pos += clave.size() + 1;
   size_t fin = query.find( '&', pos );
   return UrlDecode( ( std::string::npos == fin ) ? query.substr( pos ) : query.substr( pos, fin - pos ) );
}

void task( VSocket * cliente ) {

   char buffer[ BUFSIZE ] = { 0 };
   cliente->Read( buffer, BUFSIZE - 1 );
   std::string request( buffer );

   bitacora->log( request, Intermediario );

   size_t inicioPath = request.find( ' ' ) + 1;
   size_t finPath = request.find( ' ', inicioPath );
   std::string pathCompleto = ( std::string::npos != finPath ) ? request.substr( inicioPath, finPath - inicioPath ) : "/";

   std::string path = pathCompleto;
   std::string query;
   size_t q = pathCompleto.find( '?' );
   if ( std::string::npos != q ) { path = pathCompleto.substr( 0, q ); query = pathCompleto.substr( q + 1 ); }

   std::string cli = ObtenerParametro( query, "cli", "CLI_04" );

   std::string html;

   if ( "/" == path ) {

      html = PaginaInicio( cli );

   } else if ( 0 == path.find( "/categorias" ) ) {

      html = PaginaCategorias( cli );

   } else if ( 0 == path.find( "/productos/" ) ) {

      std::string categoria = UrlDecode( path.substr( strlen( "/productos/" ) ) );
      html = PaginaProductos( categoria, cli );

   } else if ( 0 == path.find( "/agregar/" ) ) {

      std::string resto = path.substr( strlen( "/agregar/" ) );
      size_t barra = resto.find( '/' );
      if ( std::string::npos == barra ) {
         html = HtmlPagina( "Peticion invalida", "<p class=\"err\">Uso: /agregar/producto/cantidad</p>" );
      } else {
         html = PaginaAgregar( UrlDecode( resto.substr( 0, barra ) ), resto.substr( barra + 1 ), cli );
      }

   } else if ( 0 == path.find( "/factura" ) ) {

      html = PaginaFactura( cli );

   } else if ( 0 == path.find( "/shutdown" ) ) {

      html = HtmlPagina( "Cerrando", "<p>Intermediario apagandose...</p>" );
      cliente->Write( HttpResponder( 200, "OK", html ).c_str() );
      cliente->Close();
      bitacora->log( "Intermediario cerrado por solicitud", Intermediario );
      delete cliente;
      exit( 0 );

   } else {

      cliente->Write( HttpResponder( 404, "Not Found", HtmlPagina( "No encontrado", "<p>Ruta invalida.</p>" ) ).c_str() );
      cliente->Close();
      delete cliente;
      return;

   }

   cliente->Write( HttpResponder( 200, "OK", html ).c_str() );
   cliente->Close();
   delete cliente;

}


void ManejarCierre( int senal ) {
   std::cout << "\n[INTERMEDIARIO] Cerrando ordenadamente...\n";
   if ( nullptr != descubrimiento ) {
      descubrimiento->AnunciarMuerte();
      bitacora->log( "DEATH enviado antes de cerrar", Intermediario );
   }
   if ( nullptr != s1Global ) delete s1Global;
   exit( 0 );
}


int main( int argc, char ** argv ) {

   if ( argc > 1 ) MI_ID = argv[ 1 ];
   if ( argc > 2 ) PUERTO_HTTP = atoi( argv[ 2 ] );
   if ( argc > 3 ) PUERTO_MULTICAST = atoi( argv[ 3 ] );
   bool usarSSL = ( argc > 4 && 0 == strcmp( argv[ 4 ], "ssl" ) );

   bitacora = new Logger( "./bitacora_intermediario_web_" + MI_ID + ".log" );

   VSocket * s1;
   if ( usarSSL ) {
      s1 = new SSLSocket( (char *) "ci0123.pem", (char *) "key0123.pem" );
   } else {
      s1 = new Socket( 's' );
   }
   s1Global = s1;
   s1->Bind( PUERTO_HTTP );
   s1->MarkPassive( 10 );

   descubrimiento = new Descubrimiento( MI_ID, "127.0.0.1", PUERTO_HTTP, PUERTO_MULTICAST );
   descubrimiento->IniciarEscucha();
   descubrimiento->AnunciarPresencia();

   signal( SIGINT, ManejarCierre );

   std::cout << "[INTERMEDIARIO WEB " << MI_ID << "] " << ( usarSSL ? "https" : "http" ) << "://127.0.0.1:" << PUERTO_HTTP << "/\n";
   bitacora->log( "Intermediario web " + MI_ID + " iniciado en el puerto " + std::to_string( PUERTO_HTTP ) + ( usarSSL ? " (SSL)" : "" ), Intermediario );

   std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

   for ( ; ; ) {
      VSocket * cliente = s1->AcceptConnection();
      std::thread worker( task, cliente );
      worker.detach();
   }

   return 0;

}
