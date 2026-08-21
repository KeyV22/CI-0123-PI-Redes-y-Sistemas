/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  ****** UDP IPv4 test
  *
  * (Fedora version)
  *
  *   Client side implementation of UDP client-server model 
  *
 **/

#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

#include "VSocket.h"
#include "Socket.h"

#define PORT    1234 
#define MAXLINE 1024 

int main() {
   VSocket * client;
   int sockfd; 
   int n, len; 
   char buffer[MAXLINE]; 
   char *hello = (char *) "Hello 2026-ii from CI0123 client"; 
   struct sockaddr_in6 other;

   client = new Socket( 'd' );	// Creates an UDP socket: datagram

   memset( &other, 0, sizeof( other ) ); 
   
   other.sin6_family = AF_INET6; 
   other.sin6_port = htons( PORT ); 
   n = inet_pton( AF_INET6, "10.1.35.50", &other.sin6_addr );	// IP address to test our client with a Python server on lab 3-5
   //n = inet_pton( AF_INET6, "127.0.0.1", &other.sin6_addr );
   if ( 1 != n ) {
      printf( "Error converting from IP address\n" );
      exit( 23 );
   }

   n = client->sendTo( (void *) hello, strlen( hello ), (void *) & other ); 
   printf("Client: Hello message sent.\n"); 
   
   n = client->recvFrom( (void *) buffer, MAXLINE, (void *) & other );
   buffer[n] = '\0'; 
   printf("Client message received: %s\n", buffer); 

   client->Close(); 

   return 0;
 
} 

