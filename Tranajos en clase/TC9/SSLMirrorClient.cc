/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *   SSL Socket client example
  *
  *  Uso:
  *     ./SSLMirrorClient.out                          -> IPv4, 127.0.0.1, mensaje por defecto
  *     ./SSLMirrorClient.out "mensaje"                 -> IPv4, 127.0.0.1, mensaje custom
  *     ./SSLMirrorClient.out 6 <direccion-ipv6> "msj"  -> IPv6
  *
 **/

#include <stdio.h>
#include <cstring>

#include "VSocket.h"
#include "SSLSocket.h"

#define PORT "4321"
#define BUFSIZE 1024

int main( int argc, char ** argv ) {
   SSLSocket * s;
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

   s = new SSLSocket( ipv6 );
   memset( buffer, 0, BUFSIZE );

   s->Connect( host, PORT );

   printf( "Conectado con cifrado: %s\n", s->GetCipher() );
   s->ShowCerts();

   s->Write( mensaje );
   size_t leidos = s->Read( buffer, BUFSIZE - 1 );
   buffer[ leidos ] = '\0';
   printf( "Recibido: \"%s\"\n", buffer );

   delete s;

   return 0;

}
