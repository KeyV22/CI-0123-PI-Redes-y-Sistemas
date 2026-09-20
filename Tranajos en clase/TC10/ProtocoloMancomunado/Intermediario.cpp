/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *
  *  TicAmazon - Intermediario, protocolo mancomunado
  *
  *  Decisiones de diseno:
  *
  *    - Capa Cliente<->Intermediario: HTTP, con el mensaje del protocolo
  *      mancomunado como CUERPO del POST (tal como exige el documento: "El
  *      cuerpo de las peticiones y respuestas HTTP transporta los mensajes
  *      definidos en este protocolo"). Se siguen los CODIGOS DE LA TABLA de
  *      la seccion (a) para esta capa (10/11 categorias, 20/26 productos,
  *      30/32/33 carrito, 40/41/42 factura), porque el ejemplo de flujo
  *      introductorio reutiliza el codigo 10 con un significado distinto y
  *      el codigo 22 en vez de 11/26 -- una inconsistencia del documento
  *      que hay que resolver de alguna forma consistente.
  *
  *    - Capa Intermediario<->Bodega: protocolo directo (TCP, sin HTTP), con
  *      los codigos de los FLUJOS DETALLADOS con ejemplo (10/11 categorias,
  *      21/22 productos, 24/25/27 detalle, 50 reservar) -- son los que ya
  *      se implementaron y probaron en ServidorProductos.cpp.
  *
  *    - Carrito por cliente: como HTTP aqui no mantiene una conexion
  *      persistente (cada mensaje es una conexion nueva), el carrito de
  *      cada cliente se guarda en el Intermediario indexado por su ID de
  *      origen (CLI_04, etc.)
  *
  *    - Redundancia de bodegas: el Intermediario descubre TODAS las
  *      instancias que se anuncian como "SERV" (pueden ser varias, ver nota
  *      en Descubrimiento.hpp) y recuerda en una cache cual bodega tiene
  *      cada producto, para poder mandarle la reserva (50) a la bodega
  *      correcta despues.
  *
  *    - Reintentos / error 92: si ninguna bodega conocida responde tras
  *      REINTENTOS_MAX intentos, se le devuelve al cliente el mensaje de
  *      error 92 (fallo de comunicacion)
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

#include "Socket.h"
#include "Logger.hpp"
#include "Protocolo.hpp"
#include "Descubrimiento.hpp"

#define BUFSIZE 2048
#define REINTENTOS_MAX 2

static std::string MI_ID = "INT_04";
static int PUERTO_HTTP = 8080;
static int PUERTO_MULTICAST = 5000;

static Logger * bitacora = nullptr;
static Descubrimiento * descubrimiento = nullptr;
static VSocket * s1Global = nullptr;

struct ItemCarrito { std::string nombre; std::string precio; int cantidad; };
static std::map<std::string, std::vector<ItemCarrito>> carritos;   // clienteId -> items
static std::mutex carritosMutex;

static std::map<std::string, InfoPeer> productoEnBodega;   // nombre de producto -> bodega que lo tiene (cache)
static std::mutex cacheMutex;


// ===== comunicacion con la(s) bodega(s) =====

/**
  *  EnviarABodega
  *     Abre una conexion nueva a una bodega puntual (ip:puerto), manda el
  *     mensaje, y devuelve la respuesta cruda. Lanza excepcion si falla.
  *
 **/
std::string EnviarABodega( const InfoPeer & bodega, const std::string & mensaje ) {

   VSocket * conexion = new Socket( 's' );
   conexion->Connect( bodega.ip.c_str(), std::to_string( bodega.puerto ).c_str() );

   conexion->Write( mensaje.c_str() );

   char buffer[ BUFSIZE ];
   size_t leidos = conexion->Read( buffer, BUFSIZE - 1 );
   buffer[ leidos ] = '\0';

   delete conexion;

   return std::string( buffer );

}


/**
  *  ConsultarTodasLasBodegas
  *     Manda el mismo mensaje a CADA bodega conocida (todas las instancias
  *     de "SERV"), con reintentos, hasta encontrar una que responda algo
  *     util (segun "EsUtil"). Si ninguna responde tras agotar los
  *     reintentos en todas, devuelve false (el llamador arma el error 92).
  *
 **/
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

            // respondio, pero sin lo que buscabamos (ej: categoria vacia en esa bodega) -- probar la siguiente bodega
            *respuestaOut = resp;
            if ( nullptr != bodegaQueRespondioOut ) *bodegaQueRespondioOut = bodega;
            break;   // no reintentar la MISMA bodega si ya contesto (aunque sin lo buscado); pasar a la siguiente

         } catch ( ... ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            continue;   // reintentar esta misma bodega
         }

      }

   }

   return false;

}


// ===== utilidades HTTP =====

std::string HttpResponder( const std::string & cuerpo ) {
   std::ostringstream resp;
   resp << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: text/plain; charset=utf-8\r\n"
        << "Content-Length: " << cuerpo.size() << "\r\n"
        << "Connection: close\r\n\r\n"
        << cuerpo;
   return resp.str();
}


// ===== manejadores de cada tipo de mensaje del protocolo (capa Cliente<->Intermediario) =====

std::string ManejarRequestCategory( const MensajeV2 & m ) {

   auto bodegas = descubrimiento->BuscarTodos( "SERV" );

   if ( bodegas.empty() ) {
      return construirError92( MI_ID, m.origen, "SERV", TipoMensaje::REQUEST_CATEGORY, REINTENTOS_MAX );
   }

   std::vector<std::string> categoriasUnicas;
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
      } catch ( ... ) {
         continue;   // esta bodega no respondio; seguir con las demas
      }
   }

   if ( !algunaRespondio ) {
      return construirError92( MI_ID, m.origen, "SERV", TipoMensaje::REQUEST_CATEGORY, REINTENTOS_MAX );
   }

   std::ostringstream lista;
   for ( size_t i = 0; i < categoriasUnicas.size(); i++ ) {
      if ( i > 0 ) lista << ",";
      lista << categoriasUnicas[ i ];
   }

   return construirMensaje( MI_ID, m.origen, TipoMensaje::CATEGORY_LIST_INT, { lista.str() } );

}


std::string ManejarRequestProducts( const MensajeV2 & m ) {

   std::string categoria = m.campos.empty() ? "" : m.campos[ 0 ];

   ResultadoValidacion r;
   if ( !ValidarCampoPorTipo( "categoria", categoria, &r ) ) {
      return r.esErrorTamano
           ? construirError91( MI_ID, m.origen, m.tipo, "categoria", r.tamanoRecibido, r.tamanoMaximo )
           : construirError90( MI_ID, m.origen, m.tipo, "categoria", categoria );
   }

   std::string resp;
   InfoPeer bodegaConLaCategoria;
   bool ok = ConsultarTodasLasBodegas(
      construirMensaje( MI_ID, "SERV", TipoMensaje::REQUEST_PRODUCTS_B, { categoria } ),
      &resp, &bodegaConLaCategoria,
      []( const MensajeV2 & r ) {
         return TipoMensaje::PRODUCT_LIST_B == r.tipo && !r.campos.empty() && r.campos[ 0 ] != "0";
      }
   );

   if ( !ok ) {
      return construirError92( MI_ID, m.origen, "SERV", TipoMensaje::REQUEST_PRODUCTS_B, REINTENTOS_MAX );
   }

   MensajeV2 rBodega = parsearMensaje( resp );

   if ( rBodega.campos.empty() || "0" == rBodega.campos[ 0 ] ) {
      return construirMensaje( MI_ID, m.origen, TipoMensaje::NO_PRODUCTS,
         { "No hay productos disponibles en la categoria \"" + categoria + "\"" } );
   }

   // cachear en que bodega vive cada producto de la lista (para poder reservarlo despues)
   if ( rBodega.campos.size() >= 2 ) {
      std::stringstream ss( rBodega.campos[ 1 ] );
      std::string item;
      while ( std::getline( ss, item, ';' ) ) {
         size_t coma = item.find( ',' );
         if ( std::string::npos == coma ) continue;
         std::string nombreProd = item.substr( 0, coma );
         std::lock_guard<std::mutex> guard( cacheMutex );
         productoEnBodega[ nombreProd ] = bodegaConLaCategoria;
      }
   }

   return construirMensaje( MI_ID, m.origen, TipoMensaje::PRODUCT_LIST_C, rBodega.campos );

}


std::string ManejarAddToCart( const MensajeV2 & m ) {

   std::string producto = m.campos.size() > 0 ? m.campos[ 0 ] : "";
   std::string countStr = m.campos.size() > 1 ? m.campos[ 1 ] : "";

   ResultadoValidacion r;
   if ( !ValidarCampoPorTipo( "producto", producto, &r ) ) {
      return construirError90( MI_ID, m.origen, m.tipo, "producto", producto );
   }
   if ( !ValidarCampoPorTipo( "count", countStr, &r ) ) {
      return construirError90( MI_ID, m.origen, m.tipo, "count", countStr );
   }

   // ubicar la bodega que tiene este producto (cache llenada por un REQUEST_PRODUCTS previo)
   InfoPeer bodega;
   {
      std::lock_guard<std::mutex> guard( cacheMutex );
      auto it = productoEnBodega.find( producto );
      if ( productoEnBodega.end() == it ) {
         return construirMensaje( MI_ID, m.origen, TipoMensaje::PRODUCT_NOT_FOUND2, { producto } );
      }
      bodega = it->second;
   }

   // 1) pedir el detalle actual (24/25) para validar precio y stock
   std::string respDetalle;
   try {
      respDetalle = EnviarABodega( bodega, construirMensaje( MI_ID, "SERV", TipoMensaje::REQUEST_DETAIL_B, { producto } ) );
   } catch ( ... ) {
      return construirError92( MI_ID, m.origen, "SERV", TipoMensaje::REQUEST_DETAIL_B, 1 );
   }

   MensajeV2 detalle = parsearMensaje( respDetalle );
   if ( TipoMensaje::PRODUCT_NOT_FOUND == detalle.tipo ) {
      return construirMensaje( MI_ID, m.origen, TipoMensaje::PRODUCT_NOT_FOUND2, { producto } );
   }

   std::string precio = detalle.campos.size() > 1 ? detalle.campos[ 1 ] : "0";
   int stock = detalle.campos.size() > 2 ? std::stoi( detalle.campos[ 2 ] ) : 0;
   int cantidadPedida = std::stoi( countStr );

   if ( cantidadPedida > stock ) {
      return construirMensaje( MI_ID, m.origen, TipoMensaje::CART_NO_STOCK,
         { producto, std::to_string( stock ), countStr } );
   }

   // 2) reservar el stock de verdad en la bodega (50), antes de confirmarle al cliente:
   //    la factura despues NO vuelve a consultar al servidor, asi que el stock debe quedar
   //    apartado desde este momento, o dos clientes podrian confirmar la misma ultima unidad)
   std::string respReserva;
   try {
      respReserva = EnviarABodega( bodega, construirMensaje( MI_ID, "SERV", TipoMensaje::RESERVE_STOCK, { producto, countStr } ) );
   } catch ( ... ) {
      return construirError92( MI_ID, m.origen, "SERV", TipoMensaje::RESERVE_STOCK, 1 );
   }

   MensajeV2 reserva = parsearMensaje( respReserva );
   if ( TipoMensaje::PRODUCT_NOT_FOUND == reserva.tipo ) {
      return construirMensaje( MI_ID, m.origen, TipoMensaje::CART_NO_STOCK,
         { producto, std::to_string( stock ), countStr } );
   }

   // 3) guardar en el carrito de ESTE cliente (indexado por su id de origen)
   {
      std::lock_guard<std::mutex> guard( carritosMutex );
      carritos[ m.origen ].push_back( { producto, precio, cantidadPedida } );
   }

   return construirMensaje( MI_ID, m.origen, TipoMensaje::CART_OK, { producto, precio, countStr } );

}


std::string ManejarRequestFactura( const MensajeV2 & m ) {

   std::vector<ItemCarrito> items;
   {
      std::lock_guard<std::mutex> guard( carritosMutex );
      auto it = carritos.find( m.origen );
      if ( carritos.end() != it ) {
         items = it->second;
         carritos.erase( it );   // la factura cierra el pedido: se vacia el carrito
      }
   }

   if ( items.empty() ) {
      return construirMensaje( MI_ID, m.origen, TipoMensaje::CARRITO_VACIO, { "El carrito esta vacio" } );
   }

   std::ostringstream detalle;
   double total = 0;
   for ( size_t i = 0; i < items.size(); i++ ) {
      double subtotal = std::stod( items[ i ].precio ) * items[ i ].cantidad;
      total += subtotal;
      std::ostringstream sub;
      sub << std::fixed << std::setprecision( 2 ) << subtotal;
      detalle << items[ i ].nombre << "," << items[ i ].cantidad << "," << sub.str();
      if ( i + 1 < items.size() ) detalle << ";";
   }

   std::ostringstream totalStr;
   totalStr << std::fixed << std::setprecision( 2 ) << total;

   return construirMensaje( MI_ID, m.origen, TipoMensaje::FACTURA,
      { totalStr.str(), std::to_string( items.size() ), detalle.str() } );

}


std::string ProcesarMensajeCliente( const MensajeV2 & m ) {

   if ( TipoMensaje::REQUEST_CATEGORY == m.tipo )    return ManejarRequestCategory( m );
   if ( TipoMensaje::REQUEST_PRODUCTS_C == m.tipo )  return ManejarRequestProducts( m );
   if ( TipoMensaje::ADD_TO_CART == m.tipo )         return ManejarAddToCart( m );
   if ( TipoMensaje::REQUEST_FACTURA == m.tipo )     return ManejarRequestFactura( m );

   return construirError90( MI_ID, m.origen, m.tipo, "tipo_mensaje", std::to_string( m.tipo ) );

}


void task( VSocket * cliente ) {

   char buffer[ BUFSIZE ] = { 0 };
   size_t leidos = cliente->Read( buffer, BUFSIZE - 1 );
   buffer[ leidos ] = '\0';

   std::string request( buffer );
   size_t finEncabezados = request.find( "\r\n\r\n" );
   std::string cuerpo = ( std::string::npos != finEncabezados ) ? request.substr( finEncabezados + 4 ) : request;

   // quitar posible \r\n final
   while ( !cuerpo.empty() && ( '\r' == cuerpo.back() || '\n' == cuerpo.back() ) ) cuerpo.pop_back();

   bitacora->log( cuerpo, Intermediario );

   MensajeV2 m = parsearMensaje( cuerpo );

   ResultadoValidacion r;
   std::string respuesta;

   if ( -1 == m.tipo || !ValidarCampoPorTipo( "origen_destino", m.origen, &r ) ) {
      respuesta = construirError90( MI_ID, "DESCONOCIDO", 0, "origen", m.origen );
   } else {
      respuesta = ProcesarMensajeCliente( m );
   }

   bitacora->log( respuesta, Intermediario );

   cliente->Write( HttpResponder( respuesta ).c_str() );
   cliente->Close();
   delete cliente;

}


void ManejarCierre( int senal ) {

   std::cout << "\n[INTERMEDIARIO] Cerrando ordenadamente...\n";
   if ( nullptr != descubrimiento ) {
      descubrimiento->AnunciarMuerte();
      bitacora->log( "DEATH enviado antes de cerrar", Intermediario );
   }
   if ( nullptr != s1Global ) {
      delete s1Global;
   }
   exit( 0 );

}


int main( int argc, char ** argv ) {

   if ( argc > 1 ) MI_ID = argv[ 1 ];
   if ( argc > 2 ) PUERTO_HTTP = atoi( argv[ 2 ] );
   if ( argc > 3 ) PUERTO_MULTICAST = atoi( argv[ 3 ] );

   bitacora = new Logger( "./bitacora_intermediario_" + MI_ID + ".log" );

   VSocket * s1 = new Socket( 's' );
   s1Global = s1;
   s1->Bind( PUERTO_HTTP );
   s1->MarkPassive( 10 );

   descubrimiento = new Descubrimiento( MI_ID, "127.0.0.1", PUERTO_HTTP, PUERTO_MULTICAST );
   descubrimiento->IniciarEscucha();
   descubrimiento->AnunciarPresencia();

   signal( SIGINT, ManejarCierre );

   std::cout << "[INTERMEDIARIO " << MI_ID << "] Escuchando HTTP en el puerto " << PUERTO_HTTP << "\n";
   bitacora->log( "Intermediario " + MI_ID + " iniciado en el puerto " + std::to_string( PUERTO_HTTP ), Intermediario );

   // dar tiempo a que lleguen los ANNOUNCE de las bodegas ya corriendo
   std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

   for ( ; ; ) {
      VSocket * cliente = s1->AcceptConnection();
      std::thread worker( task, cliente );
      worker.detach();
   }

   return 0;

}
