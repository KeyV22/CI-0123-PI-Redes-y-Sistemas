// Programa 1: BUSCA por broadcast.
// Manda (SendTo) una solicitud a la direccion de broadcast de la isla y espera
// (RecvFrom) las ofertas. No usa TCP porque aun no se conoce la IP del destino.
//
//   Uso: ./buscar <broadcast> [puertoUDP=9090] [segundos=5]
//   Ej.: ./buscar 172.16.123.63 9090 5
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include "Socket.hpp"

int main( int argc, char * argv[] ) {
   if ( argc < 2 ) {
      std::cerr << "Uso: " << argv[0] << " <broadcast> [puertoUDP=9090] [segundos=5]\n";
      return 1;
   }
   std::string broadcast = argv[1];
   int puerto = ( argc > 2 ) ? std::atoi( argv[2] ) : 9090;
   int segundos = ( argc > 3 ) ? std::atoi( argv[3] ) : 5;

   try {
      Socket udp( 'd', false );
      udp.SetBroadcast( true );     // sin esto el SO rechaza enviar a un broadcast
      udp.SetReadTimeout( 1 );      // RecvFrom devuelve 0 cada segundo sin datos

      std::string solicitud = "DISCOVER/SERVIDOR";
      std::cout << "Enviando \"" << solicitud << "\" a " << broadcast << ":" << puerto << std::endl;
      udp.SendTo( solicitud.c_str(), broadcast.c_str(), puerto );

      int encontrados = 0;
      auto limite = std::chrono::steady_clock::now() + std::chrono::seconds( segundos );
      while ( std::chrono::steady_clock::now() < limite ) {
         char buffer[256];
         std::string ipOrigen;
         int puertoOrigen = 0;
         size_t n = udp.RecvFrom( buffer, sizeof( buffer ) - 1, &ipOrigen, &puertoOrigen );
         if ( 0 == n ) {
            continue;   // timeout: se vuelve a revisar el tiempo limite
         }
         buffer[n] = '\0';
         std::cout << "Respuesta de " << ipOrigen << ":" << puertoOrigen << " -> " << buffer << std::endl;
         encontrados++;
      }
      std::cout << encontrados << " respuesta(s) en " << segundos << " s." << std::endl;
      return encontrados > 0 ? 0 : 2;
   } catch ( const std::exception & e ) {
      std::cerr << "Error: " << e.what() << "\n";
      return 1;
   }
}