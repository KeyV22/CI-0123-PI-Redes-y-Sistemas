/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *
  *  TicAmazon - Simulacion del protocolo MANCOMUNADO con hilos
  *
  *  Version adaptada de la simulacion anterior: ahora los 3 hilos (Cliente,
  *  Intermediario, Bodega) hablan el protocolo mancomunado del equipo
  *  (ORIGEN|DESTINO/TIPO/campos, con los codigos definidos en Protocolo.hpp)
  *  en vez del protocolo propio simplificado que se uso antes. La logica de
  *  negocio (que hace cada mensaje contra el FileSystem) es la misma que ya
  *  se probo en ServidorProductos.cpp/Intermediario.cpp reales; aqui solo
  *  cambia el medio de transporte: colas en memoria (Buzon) en vez de
  *  sockets TCP/HTTP, para poder demostrar el protocolo sin necesitar levantar
  *  procesos por separado.
  *
 **/

#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <vector>
#include <cstring>
#include <iomanip>

#include "FileSystem.hpp"
#include "Logger.hpp"
#include "Protocolo.hpp"

class Buzon {
   public:
      void enviar( const std::string & linea ) {
         std::lock_guard<std::mutex> lock( mtx_ );
         cola_.push( linea );
         cv_.notify_one();
      }
      std::string recibir() {
         std::unique_lock<std::mutex> lock( mtx_ );
         cv_.wait( lock, [ this ] { return !cola_.empty(); } );
         std::string m = cola_.front();
         cola_.pop();
         return m;
      }
   private:
      std::queue<std::string> cola_;
      std::mutex mtx_;
      std::condition_variable cv_;
};

static Buzon buzonHaciaIntermediario;
static Buzon buzonHaciaCliente;
static Buzon buzonHaciaBodega;
static Buzon buzonHaciaIntermediarioDesdeBodega;

static Logger bitacora( "./bitacora_simulacion_mancomunada.log" );

static const std::string MI_ID_CLIENTE = "CLI_04";
static const std::string MI_ID_INTERMEDIARIO = "INT_04";

static std::map<std::string, uint8_t> catalogoIds = { { "Alimentos", 1 }, { "Bloques", 2 } };

struct ItemCarrito { std::string nombre; std::string precio; int cantidad; };
static std::map<std::string, std::vector<ItemCarrito>> carritos;


std::string FormatearPrecio( uint32_t centavos ) {
   std::ostringstream out;
   out << std::fixed << std::setprecision( 2 ) << ( centavos / 100.0 );
   return out.str();
}

bool BuscarProductoPorNombre( FileSystem & fs, const std::string & nombre, Producto * out ) {
   for ( auto & entrada : catalogoIds ) {
      for ( auto & p : fs.listarProductos( entrada.second ) ) {
         if ( std::string( p.nombre ) == nombre ) { *out = p; return true; }
      }
   }
   return false;
}


// ===== Hilo Bodega: resuelve cada mensaje del protocolo contra el FileSystem real =====

void hiloServidorProductos( FileSystem * fs, int mensajesEsperados ) {

   for ( int i = 0; i < mensajesEsperados; i++ ) {

      std::string linea = buzonHaciaBodega.recibir();
      bitacora.log( linea, ServidorProductos );

      MensajeV2 m = parsearMensaje( linea );
      std::string respuesta;

      if ( TipoMensaje::REQUEST_CATEGORY == m.tipo ) {

         std::ostringstream lista;
         bool primero = true;
         for ( auto & c : catalogoIds ) { if ( !primero ) lista << ","; lista << c.first; primero = false; }
         respuesta = construirMensaje( "SERV", m.origen, TipoMensaje::CATEGORY_LIST_INT, { lista.str() } );

      } else if ( TipoMensaje::REQUEST_PRODUCTS_B == m.tipo ) {

         std::string categoria = m.campos.empty() ? "" : m.campos[ 0 ];
         auto it = catalogoIds.find( categoria );
         if ( catalogoIds.end() == it ) {
            respuesta = construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_LIST_B, { "0" } );
         } else {
            auto productos = fs->listarProductos( it->second );
            std::ostringstream lista;
            for ( size_t j = 0; j < productos.size(); j++ ) {
               lista << productos[ j ].nombre << "," << FormatearPrecio( productos[ j ].precio )
                     << "," << productos[ j ].cantidadDisponible;
               if ( j + 1 < productos.size() ) lista << ";";
            }
            respuesta = construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_LIST_B,
                                           { std::to_string( productos.size() ), lista.str() } );
         }

      } else if ( TipoMensaje::REQUEST_DETAIL_B == m.tipo ) {

         std::string nombre = m.campos.empty() ? "" : m.campos[ 0 ];
         Producto p;
         if ( BuscarProductoPorNombre( *fs, nombre, &p ) ) {
            respuesta = construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_DETAIL,
               { p.nombre, FormatearPrecio( p.precio ), std::to_string( p.cantidadDisponible ), "Producto de la bodega TicAmazon" } );
         } else {
            respuesta = construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_NOT_FOUND, { nombre } );
         }

      } else if ( TipoMensaje::RESERVE_STOCK == m.tipo ) {

         std::string nombre = m.campos.size() > 0 ? m.campos[ 0 ] : "";
         std::string cantStr = m.campos.size() > 1 ? m.campos[ 1 ] : "";
         Producto p;
         if ( BuscarProductoPorNombre( *fs, nombre, &p ) ) {
            std::string error;
            if ( fs->reservarProducto( p.idProducto, (uint32_t) std::stoul( cantStr ), &error ) ) {
               respuesta = construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_DETAIL, { nombre, "reservado", cantStr } );
            } else {
               respuesta = construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_NOT_FOUND, { nombre } );
            }
         } else {
            respuesta = construirMensaje( "SERV", m.origen, TipoMensaje::PRODUCT_NOT_FOUND, { nombre } );
         }

      } else {
         respuesta = construirError90( "SERV", m.origen, m.tipo, "tipo_mensaje", std::to_string( m.tipo ) );
      }

      bitacora.log( respuesta, ServidorProductos );
      buzonHaciaIntermediarioDesdeBodega.enviar( respuesta );

   }

}


// ===== Hilo Intermediario: recibe del cliente, resuelve contra la bodega, arma el carrito =====

void hiloIntermediario( int mensajesEsperados ) {

   for ( int i = 0; i < mensajesEsperados; i++ ) {

      std::string peticion = buzonHaciaIntermediario.recibir();
      bitacora.log( peticion, Intermediario );

      MensajeV2 m = parsearMensaje( peticion );
      std::string respuesta;

      if ( TipoMensaje::REQUEST_CATEGORY == m.tipo ) {

         buzonHaciaBodega.enviar( construirMensaje( MI_ID_INTERMEDIARIO, "SERV", TipoMensaje::REQUEST_CATEGORY, {} ) );
         std::string respBodega = buzonHaciaIntermediarioDesdeBodega.recibir();
         MensajeV2 rb = parsearMensaje( respBodega );
         respuesta = construirMensaje( MI_ID_INTERMEDIARIO, m.origen, TipoMensaje::CATEGORY_LIST_INT, rb.campos );

      } else if ( TipoMensaje::REQUEST_PRODUCTS_C == m.tipo ) {

         std::string categoria = m.campos.empty() ? "" : m.campos[ 0 ];
         buzonHaciaBodega.enviar( construirMensaje( MI_ID_INTERMEDIARIO, "SERV", TipoMensaje::REQUEST_PRODUCTS_B, { categoria } ) );
         std::string respBodega = buzonHaciaIntermediarioDesdeBodega.recibir();
         MensajeV2 rb = parsearMensaje( respBodega );

         if ( rb.campos.empty() || "0" == rb.campos[ 0 ] ) {
            respuesta = construirMensaje( MI_ID_INTERMEDIARIO, m.origen, TipoMensaje::NO_PRODUCTS,
               { "No hay productos disponibles en la categoria \"" + categoria + "\"" } );
         } else {
            respuesta = construirMensaje( MI_ID_INTERMEDIARIO, m.origen, TipoMensaje::PRODUCT_LIST_C, rb.campos );
         }

      } else if ( TipoMensaje::ADD_TO_CART == m.tipo ) {

         std::string producto = m.campos.size() > 0 ? m.campos[ 0 ] : "";
         std::string cantStr = m.campos.size() > 1 ? m.campos[ 1 ] : "";

         buzonHaciaBodega.enviar( construirMensaje( MI_ID_INTERMEDIARIO, "SERV", TipoMensaje::REQUEST_DETAIL_B, { producto } ) );
         std::string respDetalle = buzonHaciaIntermediarioDesdeBodega.recibir();
         MensajeV2 detalle = parsearMensaje( respDetalle );

         if ( TipoMensaje::PRODUCT_NOT_FOUND == detalle.tipo ) {
            respuesta = construirMensaje( MI_ID_INTERMEDIARIO, m.origen, TipoMensaje::PRODUCT_NOT_FOUND2, { producto } );
         } else {
            std::string precio = detalle.campos.size() > 1 ? detalle.campos[ 1 ] : "0";
            int stock = detalle.campos.size() > 2 ? std::stoi( detalle.campos[ 2 ] ) : 0;
            int cantidadPedida = std::stoi( cantStr );

            if ( cantidadPedida > stock ) {
               respuesta = construirMensaje( MI_ID_INTERMEDIARIO, m.origen, TipoMensaje::CART_NO_STOCK,
                  { producto, std::to_string( stock ), cantStr } );
            } else {
               buzonHaciaBodega.enviar( construirMensaje( MI_ID_INTERMEDIARIO, "SERV", TipoMensaje::RESERVE_STOCK, { producto, cantStr } ) );
               buzonHaciaIntermediarioDesdeBodega.recibir();   // confirmacion de reserva (no se necesita el contenido)

               carritos[ m.origen ].push_back( { producto, precio, cantidadPedida } );
               respuesta = construirMensaje( MI_ID_INTERMEDIARIO, m.origen, TipoMensaje::CART_OK, { producto, precio, cantStr } );
            }
         }

      } else if ( TipoMensaje::REQUEST_FACTURA == m.tipo ) {

         auto it = carritos.find( m.origen );
         if ( carritos.end() == it || it->second.empty() ) {
            respuesta = construirMensaje( MI_ID_INTERMEDIARIO, m.origen, TipoMensaje::CARRITO_VACIO, { "El carrito esta vacio" } );
         } else {
            std::ostringstream detalle;
            double total = 0;
            auto & items = it->second;
            for ( size_t j = 0; j < items.size(); j++ ) {
               double subtotal = std::stod( items[ j ].precio ) * items[ j ].cantidad;
               total += subtotal;
               std::ostringstream sub; sub << std::fixed << std::setprecision( 2 ) << subtotal;
               detalle << items[ j ].nombre << "," << items[ j ].cantidad << "," << sub.str();
               if ( j + 1 < items.size() ) detalle << ";";
            }
            std::ostringstream totalStr; totalStr << std::fixed << std::setprecision( 2 ) << total;
            respuesta = construirMensaje( MI_ID_INTERMEDIARIO, m.origen, TipoMensaje::FACTURA,
               { totalStr.str(), std::to_string( items.size() ), detalle.str() } );
            carritos.erase( it );
         }

      }

      bitacora.log( respuesta, Intermediario );
      buzonHaciaCliente.enviar( respuesta );

   }

}


// ===== Hilo Cliente: simula la interaccion, hablando el protocolo mancomunado =====

void hiloCliente() {

   auto pedir = []( const std::string & msg ) {
      bitacora.log( msg, Cliente );
      buzonHaciaIntermediario.enviar( msg );
      std::string resp = buzonHaciaCliente.recibir();
      bitacora.log( resp, Cliente );
      return resp;
   };

   std::cout << "[CLIENTE] Pidiendo categorias...\n";
   std::cout << pedir( construirMensaje( MI_ID_CLIENTE, MI_ID_INTERMEDIARIO, TipoMensaje::REQUEST_CATEGORY, {} ) ) << "\n";

   std::cout << "[CLIENTE] Pidiendo productos de \"Alimentos\"...\n";
   std::cout << pedir( construirMensaje( MI_ID_CLIENTE, MI_ID_INTERMEDIARIO, TipoMensaje::REQUEST_PRODUCTS_C, { "Alimentos" } ) ) << "\n";

   std::cout << "[CLIENTE] Agregando 2 unidades de CafeFrio al carrito...\n";
   std::cout << pedir( construirMensaje( MI_ID_CLIENTE, MI_ID_INTERMEDIARIO, TipoMensaje::ADD_TO_CART, { "CafeFrio", "2" } ) ) << "\n";

   std::cout << "[CLIENTE] Pidiendo factura proforma...\n";
   std::cout << pedir( construirMensaje( MI_ID_CLIENTE, MI_ID_INTERMEDIARIO, TipoMensaje::REQUEST_FACTURA, {} ) ) << "\n";

}


int main() {

   std::remove( "bodega_simulacion_mancomunada.dat" );
   FileSystem fs( "bodega_simulacion_mancomunada.dat" );

   auto crear = []( uint32_t id, const char * nombre, uint32_t precioCentavos, uint32_t stock ) {
      Producto p{};
      p.idProducto = id; p.precio = precioCentavos; p.cantidadDisponible = stock;
      memset( p.nombre, 0, 20 );
      strncpy( p.nombre, nombre, 19 );
      return p;
   };

   fs.agregarProducto( 1, "Alimentos", crear( 101, "CafeFrio", 325, 40 ) );
   fs.agregarProducto( 1, "Alimentos", crear( 102, "Tresleches", 380, 15 ) );
   fs.agregarProducto( 2, "Bloques", crear( 201, "BloqueRojo", 300, 120 ) );

   const int CANTIDAD_MENSAJES = 4;

   std::thread hServidor( hiloServidorProductos, &fs, CANTIDAD_MENSAJES );
   std::thread hIntermediario( hiloIntermediario, CANTIDAD_MENSAJES );
   std::thread hCliente( hiloCliente );

   hCliente.join();
   hIntermediario.join();
   hServidor.join();

   std::cout << "\n[SIMULACION] Completa (protocolo mancomunado). Ver bitacora_simulacion_mancomunada.log\n";

   return 0;

}
