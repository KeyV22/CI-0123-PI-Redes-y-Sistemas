#include <cstdlib>
#include <iostream>
#include <string>
#include "ServicioProductosDistribuido.hpp"
#include "Servidor.hpp"
#include "Descubrimiento.hpp"
#include "Broadcasts.hpp"
#include <csignal>


// Uso: ./intermediario [puerto] [broadcast] [host:puerto ...]
//   Un argumento SIN ':' es la direccion de broadcast; con ':' es una semilla.
//   ej: ./intermediario 8080 127.255.255.255
//   ej: ./intermediario 8080 172.16.123.63 192.168.1.10:9090
int main( int argc, char * argv[] ) {
   signal( SIGPIPE, SIG_IGN );
   
   int puerto = ( argc > 1 ) ? std::atoi( argv[1] ) : 8080;
   std::string broadcast;

   ServicioProductosDistribuido servicio;
   for ( int i = 2; i < argc; i++ ) {
      std::string arg = argv[i];
      size_t dp = arg.rfind( ':' );
      if ( dp == std::string::npos ) {
         broadcast = arg;
         continue;
      }
      int p = std::atoi( arg.substr( dp + 1 ).c_str() );
      if ( p <= 0 ) {
         std::cerr << "Argumento invalido (use host:puerto): " << arg << "\n";
         return 1;
      }
      servicio.AgregarSemilla( arg.substr( 0, dp ), p );
   }

   Servidor servidor( puerto );
   if ( !servidor.Iniciar() ) {
      return 1;
   }
   servicio.IniciarMonitor( 10 );

   Descubrimiento descubrimiento( servicio, puerto, ResolverBroadcasts( broadcast ) );
   descubrimiento.Iniciar( 10 );

   servidor.Ejecutar( servicio );
   return 0;
}