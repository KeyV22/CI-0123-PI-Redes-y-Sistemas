#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "Protocolo.hpp"
#include "almacenamiento.hpp"

// ===================== Buzon (mailbox) thread-safe =====================
//
// Sustituye a los sockets: cada "enlace" de la simulacion (Cliente<->
// Intermediario, Intermediario<->Bodega) es en realidad un par de buzones,
// uno por sentido, igual que un socket full-duplex pero en memoria.

class Buzon {
   public:
      void enviar( const std::string & linea ) {
         std::lock_guard<std::mutex> guard( mtx );
         cola.push( linea );
         cv.notify_one();
      }

      std::string recibir() {
         std::unique_lock<std::mutex> guard( mtx );
         cv.wait( guard, [ this ] { return !cola.empty(); } );
         std::string linea = cola.front();
         cola.pop();
         return linea;
      }

   private:
      std::queue<std::string> cola;
      std::mutex mtx;
      std::condition_variable cv;
};

// Enlace Cliente <-> Intermediario
static Buzon buzonHaciaIntermediario;   // Cliente        -> Intermediario
static Buzon buzonDesdeIntermediario;   // Intermediario  -> Cliente

// Enlace Intermediario <-> Bodega
static Buzon buzonHaciaBodega;          // Intermediario  -> Bodega
static Buzon buzonDesdeBodega;          // Bodega         -> Intermediario

// Mensaje centinela para pedirle a un hilo que termine su ciclo de vida
// (evita tener que adivinar de antemano cuantos mensajes va a recibir
// cada hilo, que era la limitacion de la simulacion anterior).
static const std::string FIN = "__FIN__";

static const std::string ID_CLIENTE       = "CLI_04";
static const std::string ID_INTERMEDIARIO = "INT_04";
static const std::string ID_BODEGA        = "SERV";


/**
  *  logProtocolo
  *     Imprime cada linea del protocolo tal como viaja por los buzones,
  *     para poder seguir la conversacion completa en la consola (equivalente
  *     a inspeccionar el trafico de un socket real con Wireshark/tcpdump).
  *
 **/
static std::mutex logMutex;
void logProtocolo( const std::string & quien, const std::string & linea ) {
   std::lock_guard<std::mutex> guard( logMutex );
   std::cout << "[" << quien << "] " << linea << "\n";
}


// ===================== Hilo Bodega =====================
//
// Copia fiel de los manejadores de ServidorProductos.cpp. La unica
// diferencia real es que "BuscarProducto" ya no depende de un socket: se
// llama directo porque todo corre en el mismo proceso, sobre el mismo
// Almacenamiento que usaria la Bodega real.

static Almacenamiento almacen;
static std::mutex almacenMutex;

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

   return construirMensaje( ID_BODEGA, ID_INTERMEDIARIO, TipoMensaje::CATEGORY_LIST_INT, { lista.str() } );
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
      return construirMensaje( ID_BODEGA, origen, TipoMensaje::PRODUCT_LIST_B, { "0" } );
   }
   return construirMensaje( ID_BODEGA, origen, TipoMensaje::PRODUCT_LIST_B, { std::to_string( count ), lista.str() } );
}

std::string ManejarDetalle( const std::string & nombreProducto, const std::string & origen ) {
   std::lock_guard<std::mutex> guard( almacenMutex );

   ProductoTexto p;
   std::string bodega;
   if ( !BuscarProducto( nombreProducto, &p, &bodega ) ) {
      return construirMensaje( ID_BODEGA, origen, TipoMensaje::PRODUCT_NOT_FOUND, { nombreProducto } );
   }

   return construirMensaje( ID_BODEGA, origen, TipoMensaje::PRODUCT_DETAIL,
      { p.producto, p.precio, p.cantidad, "Producto de " + bodega } );
}

std::string ManejarReservar( const std::string & nombreProducto, const std::string & cantidadStr, const std::string & origen ) {
   std::lock_guard<std::mutex> guard( almacenMutex );

   ProductoTexto p;
   std::string bodega;
   if ( !BuscarProducto( nombreProducto, &p, &bodega ) ) {
      return construirMensaje( ID_BODEGA, origen, TipoMensaje::PRODUCT_NOT_FOUND, { nombreProducto } );
   }

   if ( !ValidarStock( cantidadStr ) ) {
      return construirError90( ID_BODEGA, origen, TipoMensaje::RESERVE_STOCK, "stock", cantidadStr );
   }

   int cantidad = 0;
   try {
      cantidad = std::stoi( cantidadStr );
   } catch ( ... ) {
      return construirError90( ID_BODEGA, origen, TipoMensaje::RESERVE_STOCK, "stock", cantidadStr );
   }

   if ( cantidad <= 0 ) {
      return construirError90( ID_BODEGA, origen, TipoMensaje::RESERVE_STOCK, "stock", cantidadStr );
   }

   std::string error;
   if ( !almacen.actualizar_cantidad( bodega, nombreProducto, -cantidad, &error ) ) {
      if ( "STOCK_INSUFICIENTE" == error ) {
         return construirMensaje( ID_BODEGA, origen, TipoMensaje::CART_NO_STOCK,
                                  { nombreProducto, p.cantidad, cantidadStr } );
      }
      return construirMensaje( ID_BODEGA, origen, TipoMensaje::PRODUCT_NOT_FOUND, { nombreProducto } );
   }

   return construirMensaje( ID_BODEGA, origen, TipoMensaje::PRODUCT_DETAIL, { nombreProducto, "reservado", cantidadStr } );
}


/**
  *  hiloBodega
  *     Bucle equivalente al accept()+task() de ServidorProductos.cpp, pero
  *     leyendo del buzon en vez de un socket. Termina cuando recibe el
  *     mensaje centinela FIN.
  *
 **/
void hiloBodega() {

   for ( ; ; ) {

      std::string linea = buzonHaciaBodega.recibir();
      if ( FIN == linea ) break;

      logProtocolo( "BODEGA  <-", linea );

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

      } else {

         respuesta = construirError90( ID_BODEGA, m.origen, m.tipo, "tipo_mensaje", std::to_string( m.tipo ) );

      }

      logProtocolo( "BODEGA  ->", respuesta );
      buzonDesdeBodega.enviar( respuesta );
   }
}


// ===================== Hilo Intermediario =====================
//
// Adaptacion de la logica real de Intermediario.cpp (EnviarABodega +
// PaginaAgregar/PaginaFactura), pero hablando protocolo mancomunado puro
// con el Cliente en vez de servir HTML por HTTP, y usando los buzones en
// vez de sockets hacia la Bodega. Como en la simulacion solo hay una
// Bodega (el hilo de arriba), no hace falta el descubrimiento multicast
// ni ConsultarTodasLasBodegas: se habla directo con el unico buzon de
// Bodega disponible.

struct ItemCarrito { std::string nombre; std::string precio; int cantidad; };

std::string ConsultarBodega( const std::string & mensaje ) {
   logProtocolo( "INT     ->", mensaje );
   buzonHaciaBodega.enviar( mensaje );
   std::string respuesta = buzonDesdeBodega.recibir();
   logProtocolo( "INT     <-", respuesta );
   return respuesta;
}

void hiloIntermediario() {

   std::vector<ItemCarrito> carrito;

   for ( ; ; ) {

      std::string linea = buzonHaciaIntermediario.recibir();
      if ( FIN == linea ) { buzonHaciaBodega.enviar( FIN ); break; }

      logProtocolo( "INT  <-CLI", linea );

      MensajeV2 m = parsearMensaje( linea );
      std::string respuestaCliente;

      if ( TipoMensaje::REQUEST_CATEGORY == m.tipo ) {

         // Cliente pide categorias: se reenvia igual hacia la Bodega y se
         // reenvia igual la respuesta (mismo codigo en ambos tramos, tal
         // como documenta Protocolo.hpp para CATEGORY_LIST_INT).
         std::string respBodega = ConsultarBodega( construirMensaje( ID_INTERMEDIARIO, ID_BODEGA, TipoMensaje::REQUEST_CATEGORY ) );
         MensajeV2 r = parsearMensaje( respBodega );
         respuestaCliente = construirMensaje( ID_INTERMEDIARIO, m.origen, TipoMensaje::CATEGORY_LIST_INT, r.campos );

      } else if ( TipoMensaje::REQUEST_PRODUCTS_C == m.tipo ) {

         std::string categoria = m.campos.empty() ? "" : m.campos[ 0 ];
         std::string respBodega = ConsultarBodega( construirMensaje( ID_INTERMEDIARIO, ID_BODEGA, TipoMensaje::REQUEST_PRODUCTS_B, { categoria } ) );
         MensajeV2 r = parsearMensaje( respBodega );

         if ( "0" == ( r.campos.empty() ? "" : r.campos[ 0 ] ) ) {
            respuestaCliente = construirMensaje( ID_INTERMEDIARIO, m.origen, TipoMensaje::NO_PRODUCTS,
               { "No hay productos disponibles en la categoria \"" + categoria + "\"" } );
         } else {
            respuestaCliente = construirMensaje( ID_INTERMEDIARIO, m.origen, TipoMensaje::PRODUCT_LIST_C, r.campos );
         }

      } else if ( TipoMensaje::ADD_TO_CART == m.tipo ) {

         std::string producto     = m.campos.size() > 0 ? m.campos[ 0 ] : "";
         std::string cantidadStr  = m.campos.size() > 1 ? m.campos[ 1 ] : "";
         int cantidadPedida = 0;
         try { cantidadPedida = std::stoi( cantidadStr ); } catch ( ... ) { cantidadPedida = 0; }

         std::string respDetalle = ConsultarBodega( construirMensaje( ID_INTERMEDIARIO, ID_BODEGA, TipoMensaje::REQUEST_DETAIL_B, { producto } ) );
         MensajeV2 detalle = parsearMensaje( respDetalle );

         if ( TipoMensaje::PRODUCT_DETAIL != detalle.tipo || detalle.campos.size() < 3 ) {

            respuestaCliente = construirMensaje( ID_INTERMEDIARIO, m.origen, TipoMensaje::PRODUCT_NOT_FOUND2, { producto } );

         } else {

            std::string precio = detalle.campos[ 1 ];
            int stock = 0;
            try { stock = std::stoi( detalle.campos[ 2 ] ); } catch ( ... ) { stock = 0; }

            if ( cantidadPedida > stock ) {

               // No hace falta reservar para saber que no alcanza, pero se
               // deja pasar por RESERVE_STOCK igual que hace la Bodega real
               // (que es quien tiene la ultima palabra sobre el stock, por
               // si cambio entre el detalle y la reserva).
               respuestaCliente = construirMensaje( ID_INTERMEDIARIO, m.origen, TipoMensaje::CART_NO_STOCK,
                  { producto, std::to_string( stock ), cantidadStr } );

            } else {

               std::string respReserva = ConsultarBodega( construirMensaje( ID_INTERMEDIARIO, ID_BODEGA, TipoMensaje::RESERVE_STOCK, { producto, cantidadStr } ) );
               MensajeV2 reserva = parsearMensaje( respReserva );

               if ( TipoMensaje::CART_NO_STOCK == reserva.tipo ) {

                  std::string disponible = reserva.campos.size() > 1 ? reserva.campos[ 1 ] : "0";
                  respuestaCliente = construirMensaje( ID_INTERMEDIARIO, m.origen, TipoMensaje::CART_NO_STOCK,
                     { producto, disponible, cantidadStr } );

               } else if ( TipoMensaje::PRODUCT_DETAIL == reserva.tipo && reserva.campos.size() >= 2 && "reservado" == reserva.campos[ 1 ] ) {

                  // Solo se agrega al carrito si la Bodega confirmo la
                  // reserva (misma regla que PaginaAgregar en Intermediario.cpp).
                  bool acumulado = false;
                  for ( auto & item : carrito ) {
                     if ( item.nombre == producto && item.precio == precio ) {
                        item.cantidad += cantidadPedida;
                        acumulado = true;
                        break;
                     }
                  }
                  if ( !acumulado ) carrito.push_back( { producto, precio, cantidadPedida } );

                  respuestaCliente = construirMensaje( ID_INTERMEDIARIO, m.origen, TipoMensaje::CART_OK, { producto, precio, cantidadStr } );

               } else {

                  respuestaCliente = construirMensaje( ID_INTERMEDIARIO, m.origen, TipoMensaje::PRODUCT_NOT_FOUND2, { producto } );

               }
            }
         }

      } else if ( TipoMensaje::REQUEST_FACTURA == m.tipo ) {

         if ( carrito.empty() ) {

            respuestaCliente = construirMensaje( ID_INTERMEDIARIO, m.origen, TipoMensaje::CARRITO_VACIO );

         } else {

            std::ostringstream detalle;
            double total = 0;
            for ( size_t i = 0; i < carrito.size(); i++ ) {
               double subtotal = std::stod( carrito[ i ].precio ) * carrito[ i ].cantidad;
               total += subtotal;
               char sub[ 32 ]; snprintf( sub, sizeof( sub ), "%.2f", subtotal );
               if ( i > 0 ) detalle << ";";
               detalle << carrito[ i ].nombre << "," << carrito[ i ].cantidad << "," << sub;
            }
            char totalBuf[ 32 ]; snprintf( totalBuf, sizeof( totalBuf ), "%.2f", total );

            // Nota de alcance (la misma que ya se le indico al usuario
            // sobre la version HTTP real): el Almacenamiento nuevo no
            // tiene un indice de ordenes como el FileSystem viejo, asi que
            // esta factura se arma correctamente con lo confirmado via
            // RESERVE_STOCK, pero no se vuelve a persistir como un
            // registro de orden aparte.
            respuestaCliente = construirMensaje( ID_INTERMEDIARIO, m.origen, TipoMensaje::FACTURA,
               { totalBuf, std::to_string( carrito.size() ), detalle.str() } );
         }

      } else {

         respuestaCliente = construirError90( ID_INTERMEDIARIO, m.origen, m.tipo, "tipo_mensaje", std::to_string( m.tipo ) );

      }

      logProtocolo( "INT  ->CLI", respuestaCliente );
      buzonDesdeIntermediario.enviar( respuestaCliente );
   }
}


// ===================== Hilo Cliente =====================
//
// Dispara los escenarios de prueba, uno detras de otro, esperando siempre
// la respuesta antes de seguir (igual que hace un cliente real por HTTP).

std::string PedirAIntermediario( const std::string & mensaje ) {
   logProtocolo( "CLI     ->", mensaje );
   buzonHaciaIntermediario.enviar( mensaje );
   std::string respuesta = buzonDesdeIntermediario.recibir();
   logProtocolo( "CLI     <-", respuesta );
   return respuesta;
}

void hiloCliente() {

   std::cout << "\n===== Escenario 1: listar categorias =====\n";
   PedirAIntermediario( construirMensaje( ID_CLIENTE, ID_INTERMEDIARIO, TipoMensaje::REQUEST_CATEGORY ) );

   std::cout << "\n===== Escenario 2: listar productos de \"Alimentos\" =====\n";
   PedirAIntermediario( construirMensaje( ID_CLIENTE, ID_INTERMEDIARIO, TipoMensaje::REQUEST_PRODUCTS_C, { "Alimentos" } ) );

   std::cout << "\n===== Escenario 3: agregar al carrito un producto de Bodega-1 =====\n";
   PedirAIntermediario( construirMensaje( ID_CLIENTE, ID_INTERMEDIARIO, TipoMensaje::ADD_TO_CART, { "CafeNiFrioNiCaliente", "3" } ) );

   std::cout << "\n===== Escenario 4: agregar al carrito un producto de Bodega-2 (otra bodega) =====\n";
   PedirAIntermediario( construirMensaje( ID_CLIENTE, ID_INTERMEDIARIO, TipoMensaje::ADD_TO_CART, { "PastelChoco", "1" } ) );

   std::cout << "\n===== Escenario 5: pedir mas cantidad de la que hay en stock (CART_NO_STOCK) =====\n";
   PedirAIntermediario( construirMensaje( ID_CLIENTE, ID_INTERMEDIARIO, TipoMensaje::ADD_TO_CART, { "FlanCaramelo", "9999" } ) );

   std::cout << "\n===== Escenario 6: pedir la factura =====\n";
   PedirAIntermediario( construirMensaje( ID_CLIENTE, ID_INTERMEDIARIO, TipoMensaje::REQUEST_FACTURA ) );

   // Avisa a los otros dos hilos que ya terminaron los escenarios.
   buzonHaciaIntermediario.enviar( FIN );
}


int main( int argc, char ** argv ) {

   std::string archivoData = ( argc > 1 ) ? argv[ 1 ] : "simulacion_almacenamiento.data";

   std::ifstream pruebaExiste( archivoData );
   bool archivoEraNuevo = !pruebaExiste.good();
   pruebaExiste.close();

   if ( archivoEraNuevo ) {
      almacen.crear_archivo( archivoData );
      almacen.crear_bodega( "Bodega-1" );
      almacen.insertar_producto( "Bodega-1", "Alimentos", "CafeNiFrioNiCaliente", "40", "3.25" );
      almacen.insertar_producto( "Bodega-1", "Alimentos", "Cuatroleches", "15", "3.80" );
      almacen.insertar_producto( "Bodega-1", "Bebidas", "Leche argia", "120", "3.00" );

      almacen.crear_bodega( "Bodega-2" );
      almacen.insertar_producto( "Bodega-2", "Delicias", "RatonFrito", "60", "4.00" );
      almacen.insertar_producto( "Bodega-2", "Reposteria", "PastelChoco", "12", "8.50" );
      almacen.insertar_producto( "Bodega-2", "Reposteria", "FlanCaramelo", "20", "3.00" );
   } else {
      almacen.abrir_archivo( archivoData );
   }

   std::cout << "===== Simulacion TicAmazon (hilos + protocolo mancomunado + Almacenamiento real) =====\n";
   std::cout << "Archivo de almacenamiento: " << archivoData << ( archivoEraNuevo ? " (nuevo, recien sembrado)" : " (existente)" ) << "\n";

   std::thread tBodega( hiloBodega );
   std::thread tIntermediario( hiloIntermediario );
   std::thread tCliente( hiloCliente );

   tCliente.join();
   tIntermediario.join();
   tBodega.join();

   std::cout << "\n===== Estado final del Almacenamiento (para comprobar que las reservas se escribieron en disco) =====\n";
   for ( auto & bodega : almacen.listar_bodegas() ) {
      std::cout << "-- " << bodega << " --\n";
      for ( auto & p : almacen.listar_productos( bodega ) ) {
         std::cout << "   " << p.producto << " (" << p.categoria << ") cantidad=" << p.cantidad << " precio=" << p.precio << "\n";
      }
   }

   almacen.cerrar_archivo();

   return 0;
}
