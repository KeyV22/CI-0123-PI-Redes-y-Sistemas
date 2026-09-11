/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-i
  *  Grupos: 2 y 3
  *
  ****** VSocket base class implementation
  *
  * (Fedora version)
  *
 **/

#include <sys/socket.h>
#include <arpa/inet.h>		// ntohs, htons, inet_pton
#include <stdexcept>            // runtime_error
#include <cstring>		// memset
#include <netdb.h>		// getaddrinfo, freeaddrinfo, gai_strerror
#include <unistd.h>		// close
#include <cerrno>		// errno, strerror
#include <string>

#include "VSocket.h"


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

   this->type  = t;
   this->IPv6  = IPv6;
   this->port  = 0;
   this->sockId = -1;              // por si socket() falla, Close() no intenta cerrar basura

   int domain   = IPv6 ? AF_INET6 : AF_INET;
   int sockType = ( 's' == t ) ? SOCK_STREAM : SOCK_DGRAM;

   this->sockId = socket( domain, sockType, 0 );

   if ( -1 == this->sockId ) {
      throw std::runtime_error( "VSocket::Init, socket" );
   }

}


/**
  *  Class creator (constructor) - version alterna
  *     envuelve un descriptor de socket que ya existe (el que devuelve
  *     "accept()" en el servidor). Se usa para construir la instancia que
  *     representa la conexion con un cliente ya aceptado.
  *
  *  @param     int id: descriptor de socket ya creado y conectado
  *
 **/
void VSocket::Init( int id ){

   this->sockId = id;
   this->port   = 0;
   this->type   = 's';                 // accept() solo aplica a sockets stream (TCP)

   // Se consulta al kernel la familia real de direcciones del descriptor recibido
   struct sockaddr_storage addr;
   socklen_t addrLen = sizeof( addr );

   if ( 0 == getsockname( id, (struct sockaddr *) &addr, &addrLen ) ) {
      this->IPv6 = ( AF_INET6 == addr.ss_family );
   } else {
      this->IPv6 = false;
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

   if ( this->sockId >= 0 ) {
      st = close( this->sockId );
      this->sockId = -1;           // evita un doble close
      if ( -1 == st ) {
         throw std::runtime_error( "VSocket::Close()" );
      }
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

   this->port = port;

   if ( this->IPv6 ) {

      struct sockaddr_in6 host6;
      memset( (char *) &host6, 0, sizeof( host6 ) );
      host6.sin6_family = AF_INET6;

      st = inet_pton( AF_INET6, hostip, &host6.sin6_addr );
      if ( 1 != st ) {
         throw std::runtime_error( "VSocket::TryToConnect, inet_pton" );
      }

      host6.sin6_port = htons( port );
      st = connect( this->sockId, (sockaddr *) &host6, sizeof( host6 ) );

   } else {

      struct sockaddr_in host4;
      memset( (char *) &host4, 0, sizeof( host4 ) );
      host4.sin_family = AF_INET;

      st = inet_pton( AF_INET, hostip, &host4.sin_addr );
      if ( 1 != st ) {
         throw std::runtime_error( "VSocket::TryToConnect, inet_pton" );
      }

      host4.sin_port = htons( port );
      st = connect( this->sockId, (sockaddr *) &host4, sizeof( host4 ) );

   }

   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::TryToConnect, connect" );
   }

   return st;

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

   int st = -1;
   struct addrinfo hints;
   struct addrinfo * result = nullptr;
   struct addrinfo * rp = nullptr;

   memset( &hints, 0, sizeof( hints ) );
   hints.ai_family   = this->IPv6 ? AF_INET6 : AF_INET;
   hints.ai_socktype = ( 's' == this->type ) ? SOCK_STREAM : SOCK_DGRAM;

   st = getaddrinfo( host, service, &hints, &result );
   if ( 0 != st ) {
      throw std::runtime_error( std::string( "VSocket::TryToConnect, getaddrinfo: " ) + gai_strerror( st ) );
   }

   for ( rp = result; nullptr != rp; rp = rp->ai_next ) {
      st = connect( this->sockId, rp->ai_addr, rp->ai_addrlen );
      if ( 0 == st ) {
         break;
      }
   }

   freeaddrinfo( result );

   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::TryToConnect, connect" );
   }

   return st;

}


/**
  * Bind method
  *    use "bind" Unix system call (man 3 bind) (server mode)
  *
  * @param      int port: bind a unamed socket to a port defined in sockaddr structure
  *
  *  Links the calling process to a service at port
  *
 **/
int VSocket::Bind( int port ) {

   int st = -1;

   this->port = port;

   // Permite reutilizar el puerto inmediatamente al reiniciar el servidor,
   // evitando el clasico error "Address already in use" mientras el kernel
   // todavia tiene el socket anterior en estado TIME_WAIT
   int reuse = 1;
   setsockopt( this->sockId, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );

   if ( this->IPv6 ) {

      struct sockaddr_in6 host6;
      memset( (char *) &host6, 0, sizeof( host6 ) );
      host6.sin6_family = AF_INET6;
      host6.sin6_addr   = in6addr_any;         // equivalente IPv6 de INADDR_ANY
      host6.sin6_port   = htons( port );

      st = bind( this->sockId, (struct sockaddr *) &host6, sizeof( host6 ) );

   } else {

      struct sockaddr_in host4;
      memset( (char *) &host4, 0, sizeof( host4 ) );
      host4.sin_family      = AF_INET;
      host4.sin_addr.s_addr = htonl( INADDR_ANY );
      host4.sin_port        = htons( port );

      st = bind( this->sockId, (struct sockaddr *) &host4, sizeof( host4 ) );

   }

   if ( -1 == st ) {
      throw std::runtime_error( std::string( "VSocket::Bind, bind: " ) + strerror( errno ) );
   }

   return st;

}


/**
  * MarkPassive method
  *    use "listen" Unix system call (man listen) (server mode)
  *
  * @param      int backlog: defines the maximum length to which the queue of pending connections for this socket may grow
  *
  *  Establish socket queue length
  *
 **/
int VSocket::MarkPassive( int backlog ) {

   int st = listen( this->sockId, backlog );

   if ( -1 == st ) {
      throw std::runtime_error( std::string( "VSocket::MarkPassive, listen: " ) + strerror( errno ) );
   }

   return st;

}


/**
  * WaitForConnection method
  *    use "accept" Unix system call (man 3 accept) (server mode)
  *
  *
  *  Waits for a peer connections, return a sockfd of the connecting peer
  *
 **/
int VSocket::WaitForConnection( void ) {

   int newSockId = accept( this->sockId, nullptr, nullptr );

   if ( -1 == newSockId ) {
      throw std::runtime_error( std::string( "VSocket::WaitForConnection, accept: " ) + strerror( errno ) );
   }

   return newSockId;

}


/**
  * Shutdown method
  *    use "shutdown" Unix system call (man 3 shutdown) (server mode)
  *
  *
  *  cause all or part of a full-duplex connection on the socket associated with the file descriptor socket to be shut down
  *
 **/
int VSocket::Shutdown( int mode ) {

   int st = shutdown( this->sockId, mode );

   if ( -1 == st ) {
      throw std::runtime_error( std::string( "VSocket::Shutdown, shutdown: " ) + strerror( errno ) );
   }

   return st;

}


// UDP methods

/**
  *  sendTo method
  *
  *  @param	const void * buffer: data to send
  *  @param	size_t size data size to send
  *  @param	void * addr address to send data
  *
  *  Send data to another network point (addr) without connection (Datagram)
  *
 **/
size_t VSocket::sendTo( const void * buffer, size_t size, void * addr ) {

   socklen_t addrLen = this->IPv6 ? sizeof( struct sockaddr_in6 ) : sizeof( struct sockaddr_in );

   ssize_t st = sendto( this->sockId, buffer, size, 0, (struct sockaddr *) addr, addrLen );

   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::sendTo( void *, size_t, void * )" );
   }

   return (size_t) st;

}


/**
  *  recvFrom method
  *
  *  @param	const void * buffer: data to send
  *  @param	size_t size data size to send
  *  @param	void * addr address to receive from data
  *
  *  @return	size_t bytes received
  *
  *  Receive data from another network point (addr) without connection (Datagram)
  *
 **/
size_t VSocket::recvFrom( void * buffer, size_t size, void * addr ) {

   socklen_t addrLen = this->IPv6 ? sizeof( struct sockaddr_in6 ) : sizeof( struct sockaddr_in );

   ssize_t st = recvfrom( this->sockId, buffer, size, 0, (struct sockaddr *) addr, &addrLen );

   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::recvFrom( void *, size_t, void * )" );
   }

   return (size_t) st;

}
