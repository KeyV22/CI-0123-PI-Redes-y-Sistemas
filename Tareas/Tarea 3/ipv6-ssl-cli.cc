/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  * (Fedora version)
  *
 **/

#include <stdio.h>
#include <string.h>

#include "VSocket.h"
#include "Socket.h"
#include "SSLSocket.h"

#define	MAXBUF	1024

/**
**/
int main( int argc, char * argv[] ) {
   VSocket * client;
   char a[ MAXBUF ];
   char * os = (char *) "fe80::8f5a:e2e1:7256:ffe3%enp0s31f6";
   const char* service;
   char * orchid = (char *) "GET /lego/list.php?figure=orchid&part=1 \r\nHTTP/v1.1\r\nhost: redes.ecci\r\n\r\n";
//   char * request = (char *) "GET /ci0123 HTTP/1.1\r\nhost:redes.ecci\r\n\r\n";

   if (argc > 1 ) {
      client = new SSLSocket(true);	// Create a new stream socket for IPv4
      service="https";
   } else {
      client = new Socket( 's', true );
      service="http";
   }

   memset( a, 0 , MAXBUF );
   client->Connect( os, service);
   client->Write(  (char * ) orchid, strlen( orchid ) );
   int st = client->Read( a, MAXBUF );
   printf( "Bytes read %d\n%s\n", st, a);

   delete client;

}