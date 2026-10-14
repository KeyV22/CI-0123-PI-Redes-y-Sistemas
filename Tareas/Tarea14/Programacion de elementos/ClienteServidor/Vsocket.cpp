/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *******   VSocket base class implementation
  *
  * (Fedora version)
  *
 **/

#include <sys/socket.h>
#include <arpa/inet.h>		// ntohs, htons
#include <netinet/in.h>		// INADDR_ANY, in6addr_any
#include <sys/time.h>		// struct timeval (SO_RCVTIMEO)
#include <stdexcept>            // runtime_error
#include <cstring>		// memset
#include <netdb.h>		// getaddrinfo, freeaddrinfo
#include <unistd.h>		// close
/*
#include <cstddef>
#include <cstdio>

//#include <sys/types.h>
*/
#include "Vsocket.hpp"


/**
  *  Class creator (constructor)
  *     use Unix socket system call
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
 **/
void VSocket::Init( char t, bool IPv6 ){
   this->IPv6 = IPv6;
   this->type = t;
   this-> port=0;
   this->sockId=-1;

   if(IPv6 == false){
      if(t == 's'){
         this->sockId = socket(AF_INET, SOCK_STREAM, 0);
      }else if(t == 'd'){
         this->sockId = socket(AF_INET, SOCK_DGRAM,0);
      }
   }else{
      if(t == 's'){
         this->sockId = socket(AF_INET6, SOCK_STREAM, 0);
      }else if(t == 'd'){
         this->sockId = socket(AF_INET6, SOCK_DGRAM,0);
      }
   }

   if ( -1 == this->sockId ) {
      throw std::runtime_error( "VSocket::Init, (reason)" );
   }
}




/**
  * Class destructor
  *
 **/
VSocket::~VSocket() {

   this->Close();

}


/**
  * Close method
  *    use Unix close system call (once opened a socket is managed like a file in Unix)
  *
 **/
void VSocket::Close(){
   int st = 0;

   if(this->sockId>=0){
      st=close(this->sockId);
      this->sockId=-1;
   }

   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::Close()" );
   }

}


/**
  * TryToConnect method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dot notation, example "10.84.166.62"
  * @param      int port: process address, example 80
  *
 **/
int VSocket::TryToConnect( const char * hostip, int port ) {

   int st = -1;
   this->port=port;

   if(this->IPv6 == false){   
      struct sockaddr_in host4;
      memset((char*)&host4, 0, sizeof(host4));
      host4.sin_family = AF_INET;
      st = inet_pton(AF_INET, hostip, &host4.sin_addr);
      if(-1 == st){
         throw std::runtime_error("VSocket::TryToConnect : pton");
      }
      host4.sin_port = htons(port);
      st = connect(sockId, (sockaddr *)&host4, sizeof(host4));
      if(-1 == st){
         throw std::runtime_error("Vsocket::trytoconnect : htons");
      }
   } else{
      struct sockaddr_in6  host6;
      struct sockaddr * ha;

      memset( &host6, 0, sizeof( host6 ) );
      host6.sin6_family = AF_INET6;
      st = inet_pton( AF_INET6, hostip, &host6.sin6_addr );
      if ( st <= 0 ) {
         throw std::runtime_error( "Socket::Connect( const char *, int ) [inet_pton]" );
      }
      host6.sin6_port = htons( port );
      ha = (struct sockaddr *) &host6;
      int len = sizeof( host6 );
      st = connect( sockId, ha, len );
      if ( -1 == st ) {
         throw std::runtime_error( "VSocket::TryToConnect( const char *, int ) [connect]" );
      }
   }  
   return st;

}


/**
  * TryToBind method
  *   use "bind" Unix system call, enlaza el socket a un puerto local
  *   en todas las interfaces (INADDR_ANY / in6addr_any). Valido tanto
  *   para sockets 's' (stream, servidor TCP) como 'd' (datagram, UDP).
  *
  * @param      int port: puerto local en el que el servidor va a escuchar
  *
 **/
int VSocket::TryToBind( int port ) {
   int st = -1;
   int reuse = 1;

   // Permite reiniciar el servidor sin esperar a que el SO libere el
   // puerto (evita "Address already in use" por el estado TIME_WAIT).
   setsockopt( this->sockId, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );

   this->port = port;

   if ( this->IPv6 == false ) {
      struct sockaddr_in local;
      memset( &local, 0, sizeof( local ) );
      local.sin_family = AF_INET;
      local.sin_addr.s_addr = htonl( INADDR_ANY );
      local.sin_port = htons( port );
      st = bind( this->sockId, (sockaddr *)&local, sizeof( local ) );
   } else {
      struct sockaddr_in6 local6;
      memset( &local6, 0, sizeof( local6 ) );
      local6.sin6_family = AF_INET6;
      local6.sin6_addr = in6addr_any;
      local6.sin6_port = htons( port );
      st = bind( this->sockId, (sockaddr *)&local6, sizeof( local6 ) );
   }

   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::TryToBind : bind" );
   }
   return st;
}


/**
  * SetReadTimeout method 
  *   use SO_RCVTIMEO: si no llegan datos en ese tiempo, Read() termina
  *   lanzando runtime_error en vez de bloquear para siempre.
  *
  * @param      int segundos: tiempo maximo de espera (0 desactiva el timeout)
  *
 **/
void VSocket::SetReadTimeout( int segundos ) {
   struct timeval tv;
   tv.tv_sec = segundos;
   tv.tv_usec = 0;
   if ( -1 == setsockopt( this->sockId, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof( tv ) ) ) {
      throw std::runtime_error( "VSocket::SetReadTimeout : setsockopt" );
   }
}


/**
  * TryToConnect method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dns notation, example "os.ecci.ucr.ac.cr"
  * @param      char * service: process address, example "http"
  *
 **/
int VSocket::TryToConnect( const char *host, const char *service ) {
   int st;
   struct addrinfo hints, *result, *rp;

   memset(&hints, 0, sizeof(struct addrinfo));
   if (this->IPv6) {
      hints.ai_family = AF_INET6;
   } else {
      hints.ai_family = AF_INET;
   }

   if('s'==this->type){
      hints.ai_socktype= SOCK_STREAM;
   }else{
      hints.ai_socktype= SOCK_DGRAM;
   }

   st=getaddrinfo(host,service,&hints,&result);
   if(0!=st){
      throw std::runtime_error( "VSocket::TryToConnect, getaddrinfo" );
   }

   bool connected = false;
   for(rp=result; nullptr!=rp; rp= rp->ai_next){
      st= connect(this-> sockId, rp->ai_addr, rp->ai_addrlen);
      if(0==st){
         connected = true;
         break;
      }
   }
   freeaddrinfo(result); 

   if(!connected){
      throw std::runtime_error( "VSocket::TryToConnect,connect" );
   }

   return st;


}