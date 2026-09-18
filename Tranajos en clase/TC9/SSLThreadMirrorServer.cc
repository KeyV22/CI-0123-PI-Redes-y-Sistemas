/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *   SSL Socket server example - threads version
  *
  *  Requiere un certificado autofirmado generado con:
  *     openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout key0123.pem -out ci0123.pem
  *
 **/

#include <iostream>
#include <thread>
#include <cstring>

#include "VSocket.h"
#include "SSLSocket.h"

#define PORT 4321
#define BUFSIZE 1024


/**
 *   Tarea que corre cada hilo nuevo:
 *      leer un mensaje del cliente (ya descifrado por SSLSocket::Read)
 *      devolverlo tal cual (funcion espejo)
 *
 **/
void task( SSLSocket * client ) {

   char a[ BUFSIZE ] = { 0 };

   std::cout << "[hilo] Cifrado: " << client->GetCipher() << std::endl;
   client->ShowCerts();

   size_t leidos = client->Read( a, BUFSIZE - 1 );
   a[ leidos ] = '\0';
   std::cout << "[hilo] Mensaje recibido: \"" << a << "\"" << std::endl;

   client->Write( a );
   client->Close();

   delete client;

}


int main( int argc, char ** argv ) {
   std::thread * worker;
   SSLSocket * s1, * client;
   bool ipv6 = ( argc > 1 && 0 == strcmp( argv[1], "6" ) );

   s1 = new SSLSocket( (char *) "ci0123.pem", (char *) "key0123.pem", ipv6 );

   s1->Bind( PORT );
   s1->MarkPassive( 5 );

   std::cout << "SSLThreadMirrorServer escuchando en " << ( ipv6 ? "IPv6" : "IPv4" ) << ", puerto " << PORT << std::endl;

   for ( ; ; ) {
      client = s1->Accept();		// TCP accept() + handshake SSL completo
      worker = new std::thread( task, client );
      worker->detach();
   }

   return 0;

}
