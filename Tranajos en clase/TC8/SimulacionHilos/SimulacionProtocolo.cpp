/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  TicAmazon - Simulacion del protocolo con hilos (Cliente / Intermediario / ServidorProductos)
  *
  *  A diferencia de las simulaciones anteriores (fork + buzon de mensajes
  *  System V), esta version:
  *
  *    - Usa 3 hilos reales dentro de un mismo proceso (Cliente, Intermediario,
  *      ServidorProductos), comunicados por colas thread-safe (mutex +
  *      condition_variable) en vez de sockets o IPC de procesos.
  *    - Usa el protocolo propio YA DEFINIDO para el proyecto (el mismo que
  *      habla el Intermediario.cpp/ServidorProductos.cpp reales, sobre
  *      sockets): PEDIR:CAT, PEDIR:PROD:<cat>, AGREGAR:<id>:<cant>,
  *      PEDIR:FACTURA:<items>.
  *    - Usa el FileSystem REAL del proyecto (bloques de 256 bytes,
  *      superbloque, bitmap, indices encadenables) en vez de una bodega en
  *      memoria -- las mismas funciones ManejarCat/ManejarProd/ManejarAgregar/
  *      ManejarFactura que ya usa el ServidorProductos.cpp real, solo que
  *      aqui reciben el comando de un Buzon en vez de leerlo de un socket.
  *    - Cada linea del protocolo se registra con nuestro Logger real
  *      (bitacora_simulacion.log).
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

#include "FileSystem.hpp"
#include "Logger.hpp"

/**
  *  Buzon
  *     Cola de mensajes thread-safe (mutex + condition_variable), igual que
  *     ya usamos en las simulaciones anteriores (ahi con procesos y colas de
  *     mensajes System V; aqui con hilos del mismo proceso).
  *
 **/
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

// Cliente <-> Intermediario
static Buzon buzonHaciaIntermediario;   // Cliente -> Intermediario
static Buzon buzonHaciaCliente;         // Intermediario -> Cliente

// Intermediario <-> ServidorProductos (bodega)
static Buzon buzonHaciaBodega;          // Intermediario -> ServidorProductos
static Buzon buzonHaciaIntermediarioDesdeBodega; // ServidorProductos -> Intermediario

static Logger bitacora( "./bitacora_simulacion.log" );

static std::map<std::string, uint8_t> catalogoIds = {
   { "Alimentos",  1 },
   { "Bloques",    2 },
   { "Vehiculos",  3 }
};


/**
  *  Manejadores del protocolo contra el FileSystem real -- son practicamente
  *  identicos a los del ServidorProductos.cpp real; la unica diferencia es
  *  que ahi la respuesta se manda por un VSocket, y aqui por un Buzon.
  *
 **/
std::string ManejarCat( FileSystem & fs ) {
   std::string resp = "OK\n";
   for ( auto & nombre : fs.listarCategorias() ) {
      resp += nombre + "\n";
   }
   return resp;
}

std::string ManejarProd( FileSystem & fs, const std::string & nombreCategoria ) {
   auto it = catalogoIds.find( nombreCategoria );
   if ( catalogoIds.end() == it ) {
      return "ERR:La categoria \"" + nombreCategoria + "\" no existe.\n";
   }
   std::string resp = "OK\n";
   for ( auto & p : fs.listarProductos( it->second ) ) {
      resp += std::to_string( p.idProducto ) + ";" + std::string( p.nombre ) + ";" +
              std::to_string( p.precio ) + ";" + std::to_string( p.cantidadDisponible ) + "\n";
   }
   return resp;
}

std::string ManejarAgregar( FileSystem & fs, uint32_t idProducto, uint32_t cantidad ) {
   std::string error;
   if ( fs.reservarProducto( idProducto, cantidad, &error ) ) {
      return "OK:" + std::to_string( cantidad ) + "\n";
   }
   return "ERR:" + error + "\n";
}

std::string ManejarFactura( FileSystem & fs, const std::string & itemsCrudo ) {
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
  *  hiloServidorProductos
  *     Simula la bodega: recibe comandos del intermediario, los resuelve
  *     contra el FileSystem real, y responde.
  *
 **/
void hiloServidorProductos( FileSystem * fs, int mensajesEsperados ) {

   for ( int i = 0; i < mensajesEsperados; i++ ) {

      std::string comando = buzonHaciaBodega.recibir();
      bitacora.log( comando, ServidorProductos );

      std::string respuesta;

      if ( 0 == comando.find( "PEDIR:CAT" ) ) {

         respuesta = ManejarCat( *fs );

      } else if ( 0 == comando.find( "PEDIR:PROD:" ) ) {

         respuesta = ManejarProd( *fs, comando.substr( strlen( "PEDIR:PROD:" ) ) );

      } else if ( 0 == comando.find( "AGREGAR:" ) ) {

         std::string resto = comando.substr( strlen( "AGREGAR:" ) );
         size_t sep = resto.find( ':' );
         uint32_t idProd = (uint32_t) std::stoul( resto.substr( 0, sep ) );
         uint32_t cant   = (uint32_t) std::stoul( resto.substr( sep + 1 ) );
         respuesta = ManejarAgregar( *fs, idProd, cant );

      } else if ( 0 == comando.find( "PEDIR:FACTURA:" ) ) {

         respuesta = ManejarFactura( *fs, comando.substr( strlen( "PEDIR:FACTURA:" ) ) );

      } else {

         respuesta = "ERR:comando desconocido\n";

      }

      bitacora.log( respuesta, ServidorProductos );
      buzonHaciaIntermediarioDesdeBodega.enviar( respuesta );

   }

}


/**
  *  hiloIntermediario
  *     Recibe la solicitud del cliente, la reenvia (con el mismo protocolo)
  *     a la bodega, y le devuelve la respuesta al cliente. En esta
  *     simulacion el intermediario no traduce a HTML (eso es cosa de la capa
  *     HTTP real) -- aqui se limita a mostrar su papel de intermediar.
  *
 **/
void hiloIntermediario( int mensajesEsperados ) {

   for ( int i = 0; i < mensajesEsperados; i++ ) {

      std::string peticionCliente = buzonHaciaIntermediario.recibir();
      bitacora.log( peticionCliente, Intermediario );

      buzonHaciaBodega.enviar( peticionCliente );
      std::string respuestaBodega = buzonHaciaIntermediarioDesdeBodega.recibir();

      bitacora.log( respuestaBodega, Intermediario );
      buzonHaciaCliente.enviar( respuestaBodega );

   }

}


/**
  *  hiloCliente
  *     Simula la interaccion de un cliente: ve categorias, ve productos de
  *     una categoria, agrega uno al carrito, y pide la factura proforma.
  *
 **/
void hiloCliente() {

   auto pedir = []( const std::string & comando ) {
      bitacora.log( comando, Cliente );
      buzonHaciaIntermediario.enviar( comando );
      std::string resp = buzonHaciaCliente.recibir();
      bitacora.log( resp, Cliente );
      return resp;
   };

   std::cout << "[CLIENTE] Pidiendo categorias...\n";
   std::string categorias = pedir( "PEDIR:CAT" );
   std::cout << categorias;

   std::cout << "[CLIENTE] Pidiendo productos de \"Alimentos\"...\n";
   std::string productos = pedir( "PEDIR:PROD:Alimentos" );
   std::cout << productos;

   std::cout << "[CLIENTE] Agregando 5 unidades del producto 101 al carrito...\n";
   std::string respAgregar = pedir( "AGREGAR:101:5" );
   std::cout << respAgregar;

   std::cout << "[CLIENTE] Pidiendo factura proforma...\n";
   std::string factura = pedir( "PEDIR:FACTURA:101:5" );
   std::cout << factura;

}


int main() {

   std::remove( "bodega_simulacion.dat" );   // empezar limpio en cada corrida
   FileSystem fs( "bodega_simulacion.dat" );

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
   fs.agregarProducto( 2, "Bloques", crear( 201, "Bloque 2x4 rojo", 300, 120 ) );
   fs.agregarProducto( 3, "Vehiculos", crear( 301, "Rueda pequena", 400, 60 ) );

   const int CANTIDAD_MENSAJES = 4;   // CAT, PROD, AGREGAR, FACTURA

   std::thread hServidor( hiloServidorProductos, &fs, CANTIDAD_MENSAJES );
   std::thread hIntermediario( hiloIntermediario, CANTIDAD_MENSAJES );
   std::thread hCliente( hiloCliente );

   hCliente.join();
   hIntermediario.join();
   hServidor.join();

   std::cout << "\n[SIMULACION] Completa. Revise bitacora_simulacion.log para el detalle linea por linea.\n";

   return 0;

}
