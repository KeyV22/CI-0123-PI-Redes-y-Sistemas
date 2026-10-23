// Prueba de broadcast UDP con las clases Socket/VSocket.
//   Receptor:  ./prueba_broadcast recibir [puerto]
//   Emisor:    ./prueba_broadcast enviar <ip-de-la-red> <prefijo> [puerto] [mensaje]
#include <cstdlib>
#include <iostream>
#include <string>
#include "Socket.hpp"

int main( int argc, char * argv[] ) {
   if ( argc < 2 ) {
      std::cerr << "Uso: " << argv[0] << " recibir [puerto] | enviar <ip> <prefijo> [puerto] [mensaje]\n";
      return 1;
   }
   std::string modo = argv[1];
   try {
      Socket udp( 'd', false );
      if ( modo == "recibir" ) {
         int puerto = ( argc > 2 ) ? std::atoi( argv[2] ) : 9090;
         udp.Bind( puerto );
         std::cout << "Escuchando UDP en el puerto " << puerto << " (Ctrl+C para salir)" << std::endl;
         char buffer[1024];
         while ( true ) {
            std::string ip; int pto;
            size_t n = udp.RecvFrom( buffer, sizeof( buffer ) - 1, &ip, &pto );
            buffer[n] = '\0';
            std::cout << "Recibido de " << ip << ":" << pto << " -> " << buffer << std::endl;
         }
      } else if ( modo == "enviar" && argc >= 4 ) {
         std::string destino = Socket::DireccionBroadcast( argv[2], std::atoi( argv[3] ) );
         int puerto = ( argc > 4 ) ? std::atoi( argv[4] ) : 9090;
         std::string mensaje = ( argc > 5 ) ? argv[5] : "60/0001/INT_03/172.16.123.51/9090";
         udp.SetBroadcast( true );
         udp.SendTo( mensaje.c_str(), destino.c_str(), puerto );
         std::cout << "Enviado a " << destino << ":" << puerto << " -> " << mensaje << std::endl;
      } else {
         std::cerr << "Argumentos invalidos\n";
         return 1;
      }
   } catch ( const std::exception & e ) {
      std::cerr << "Error: " << e.what() << "\n";
      return 1;
   }
   return 0;
}

/*
cd ServidorProductos
g++ -std=c++17 PruebaBroadcast.cpp Socket.cpp Vsocket.cpp -o prueba_broadcast

# En una o más computadoras de la isla:
./prueba_broadcast recibir 9090

# En otra computadora de la misma isla (la isla 3 tiene red 172.16.123.48/28):
./prueba_broadcast enviar 172.16.123.51 28 9090 "60/0001/INT_03/172.16.123.51/9090"
*/