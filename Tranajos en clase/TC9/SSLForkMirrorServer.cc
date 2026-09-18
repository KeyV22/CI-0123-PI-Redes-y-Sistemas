/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *   SSL Socket server example - fork version
  *
  *  Requiere un certificado autofirmado generado con:
  *     openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout key0123.pem -out ci0123.pem
  *
  * (Fedora version)
  *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>	// memset
#include <unistd.h>

#include "VSocket.h"
#include "SSLSocket.h"

#define PORT 4321
#define BUFSIZE 1024

int main( int argc, char ** argv ) {
   SSLSocket * s1, * s2;
   int childpid;
   char a[ BUFSIZE ];
   bool ipv6 = ( argc > 1 && 0 == strcmp( argv[1], "6" ) );

   s1 = new SSLSocket( (char *) "ci0123.pem", (char *) "key0123.pem", ipv6 );

   s1->Bind( PORT );
   s1->MarkPassive( 5 );

   printf( "SSLForkMirrorServer escuchando en %s, puerto %d\n", ipv6 ? "IPv6" : "IPv4", PORT );

   for ( ; ; ) {

      s2 = s1->Accept();		// TCP accept() + handshake SSL completo

      childpid = fork();

      if ( childpid < 0 ) {
         perror( "server: fork error" );
      } else if ( 0 == childpid ) {	// codigo del hijo

         delete s1;			// el hijo no necesita el socket de escucha
         memset( a, 0, BUFSIZE );

         printf( "[hijo %d] Cifrado: %s\n", getpid(), s2->GetCipher() );
         s2->ShowCerts();

         size_t leidos = s2->Read( a, BUFSIZE - 1 );
         a[ leidos ] = '\0';
         printf( "[hijo %d] Mensaje recibido: \"%s\"\n", getpid(), a );

         s2->Write( a );		// mismo mensaje de vuelta (espejo)

         delete s2;
         exit( 0 );

      }

      delete s2;			// el padre no necesita esta conexion, sigue esperando la siguiente

   }

   return 0;

}
