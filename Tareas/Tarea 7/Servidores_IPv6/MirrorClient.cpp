/**
 *   UCR-ECCI
 *   CI-0123 Proyecto integrador de redes y sistemas operativos
 *
 *   Socket client/server example
 *
 *   Deben determinar la dirección IP del equipo donde van a correr el servidor
 *   para hacer la conexión en ese punto (ip addr)
 *
 **/

#include <stdio.h>
#include <cstring>
#include "Socket.hpp"

#define PORT 1234
#define BUFSIZE 512

/**
 *   Uso:
 *      ./MirrorClient.out                                  -> IPv4, 127.0.0.1, mensaje por defecto
 *      ./MirrorClient.out "mensaje"                        -> IPv4, 127.0.0.1, mensaje custom
 *      ./MirrorClient.out 6 <direccion-ipv6> "mensaje"      -> IPv6, direccion indicada
 **/
int main( int argc, char ** argv ) {
   VSocket * s;
   char buffer[ BUFSIZE ];
   bool ipv6 = false;
   const char * host = "127.0.0.1";
   const char * mensaje = "Hello world 2026 ...";

   if ( argc > 1 && 0 == strcmp( argv[1], "6" ) ) {
      ipv6 = true;
      if ( argc > 2 ) host = argv[2];
      if ( argc > 3 ) mensaje = argv[3];
   } else if ( argc > 1 ) {
      mensaje = argv[1];
   }

   s = new Socket( 's', ipv6 );     // Create a new stream socket (IPv4 o IPv6)
   memset( buffer, 0, BUFSIZE );	// Zero fill buffer

   s->TryToConnect( host, PORT ); // Same port as server
   s->Write( mensaje );
   s->Read( buffer, BUFSIZE );	// Read answer sent back from server
   printf( "%s", buffer );	// Print received string, mirror example this will print same sent string

}