/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  ******   Socket class implementation
  *
  * (Fedora version)
  *
 **/

#include <sys/socket.h>         // sockaddr_in
#include <arpa/inet.h>          // ntohs
#include <unistd.h>		// write, read
#include <cstring>
#include <stdexcept>
#include <cerrno>
#include <netinet/in.h>
#include <stdio.h>		// printf

#include "Socket.hpp"		// Derived class

/**
  *  Class constructor
  *     use Unix socket system call
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
 **/
Socket::Socket( char t, bool IPv6 ){

   this->Init( t, IPv6 );      // Call base class constructor

}


/**
  *  Class destructor
  *
  *  @param     int id: socket descriptor
  *
 **/
Socket::~Socket() {

}


/**
  * Connect method
  *   use "TryToConnect" in base class
  *
  * @param      char * host: host address in dot notation, example "10.1.166.62"
  * @param      int port: process address, example 80
  *
 **/
int Socket::Connect( const char * hostip, int port ) {

   return this->TryToConnect( hostip, port );

}


/**
  * Connect method
  *   use "TryToConnect" in base class
  *
  * @param      char * host: host address in dns notation, example "os.ecci.ucr.ac.cr"
  * @param      char * service: process address, example "http"
  *
 **/
int Socket::Connect( const char *host, const char *service ) {

   return this->TryToConnect( host, service );

}




/**
  * Read method
  *   use "read" Unix system call (man 2 read)
  *
  * @param      void * buffer: buffer to store data read from socket
  * @param      int size: buffer capacity, read will stop if buffer is full
  *
 **/
size_t Socket::Read( void * buffer, size_t size ) {

   ssize_t st;
   st = read(sockId, buffer, size);
   if(-1 == st){
      throw std::runtime_error("read error");
   }else{
      return (size_t)st;
   }

}

/**
  * Write method
  *   use "write" Unix system call (man 2 write)
  *
  * @param      void * buffer: buffer to store data write to socket
  * @param      size_t size: buffer capacity, number of bytes to write
  *
 **/
size_t Socket::Write( const void * buffer, size_t size ) {
   ssize_t st;

   st = write(this->sockId, buffer, size);
   if(-1 == st){
      throw std::runtime_error("write error");
   }else {
      return (size_t)st;
   }


}


/**
  * Write method
  *   use "write" Unix system call (man 2 write)
  *
  * @param      char * text: text to write to socket
  *
 **/
size_t Socket::Write( const char * text ) {
   return this->Write((const void*)text, strlen(text));
}

//----------extension de metodos agregados para el servidor---------

/**
  * Constructor privado usado por Accept(): envuelve un
  * descriptor que el SO ya creo y conecto (el que devuelve accept()),
  * sin llamar a la syscall socket() de nuevo.
 **/
Socket::Socket( int fdExistente, bool IPv6 ) {
   this->sockId = fdExistente;
   this->IPv6 = IPv6;
   this->type = 's';
   this->port = 0;
}

/**
  * Move constructor: transfiere el file descriptor y deja
  * al objeto origen sin uno (sockId = -1), para que su destructor no
  * intente cerrar el descriptor que ya le pasamos a este objeto nuevo.
 **/
Socket::Socket( Socket && otro ) noexcept {
   this->sockId = otro.sockId;
   this->IPv6   = otro.IPv6;
   this->type   = otro.type;
   this->port   = otro.port;
   otro.sockId  = -1;
}

/**
  * Move assignment: cierra lo que este objeto tuviera antes
  * (si tenia algo abierto) y toma el descriptor del objeto origen.
 **/
Socket & Socket::operator=( Socket && otro ) noexcept {
   if ( this != &otro ) {
      if ( this->sockId >= 0 ) {
         close( this->sockId );
      }
      this->sockId = otro.sockId;
      this->IPv6   = otro.IPv6;
      this->type   = otro.type;
      this->port   = otro.port;
      otro.sockId  = -1;
   }
   return *this;
}

/**
  * Bind method
  *   use "TryToBind" en la clase base
  *
  * @param      int port: puerto local en el que el servidor va a escuchar
  *
 **/
int Socket::Bind( int port ) {
   return this->TryToBind( port );
}

/**
  * Listen method 
  *   use "listen" Unix system call (man 2 listen), solo tiene sentido
  *   para sockets de tipo stream ('s')
  *
  * @param      int backlog: cantidad maxima de conexiones pendientes en cola
  *
 **/
int Socket::Listen( int backlog ) {
   int st = listen( this->sockId, backlog );
   if ( -1 == st ) {
      throw std::runtime_error( "Socket::Listen" );
   }
   return st;
}

/**
  * Accept method
  *   use "accept" Unix system call (man 2 accept); bloquea hasta que
  *   llega una conexion entrante y devuelve un Socket nuevo ya
  *   conectado a ese cliente (el socket de escucha original sigue
  *   escuchando, sin verse afectado).
  *
 **/
Socket Socket::Accept() {
   int fdCliente = accept( this->sockId, nullptr, nullptr );
   if ( -1 == fdCliente ) {
      throw std::runtime_error( "Socket::Accept" );
   }
   return Socket( fdCliente, this->IPv6 );
}

size_t Socket::SendTo( const void * buffer, size_t size, const char * ip, int puerto ) {
   struct sockaddr_in destino;
   memset( &destino, 0, sizeof( destino ) );
   destino.sin_family = AF_INET;
   destino.sin_port = htons( puerto );
   if ( 1 != inet_pton( AF_INET, ip, &destino.sin_addr ) ) {
      throw std::runtime_error( "Socket::SendTo : inet_pton" );
   }
   ssize_t st = sendto( this->sockId, buffer, size, 0, (sockaddr *)&destino, sizeof( destino ) );
   if ( -1 == st ) {
      throw std::runtime_error( "Socket::SendTo : sendto" );
   }
   return (size_t) st;
}

size_t Socket::SendTo( const char * texto, const char * ip, int puerto ) {
   return this->SendTo( (const void *) texto, strlen( texto ), ip, puerto );
}

size_t Socket::RecvFrom( void * buffer, size_t size, std::string * ipOrigen, int * puertoOrigen ) {
   struct sockaddr_in origen;
   socklen_t largo = sizeof( origen );
   memset( &origen, 0, sizeof( origen ) );
   ssize_t st = recvfrom( this->sockId, buffer, size, 0, (sockaddr *)&origen, &largo );
   if ( -1 == st ) {
      if ( EAGAIN == errno || EWOULDBLOCK == errno ) {
         return 0;   // vencio SetReadTimeout()
      }
      throw std::runtime_error( "Socket::RecvFrom : recvfrom" );
   }
   if ( ipOrigen ) {
      char texto[INET_ADDRSTRLEN];
      inet_ntop( AF_INET, &origen.sin_addr, texto, sizeof( texto ) );
      *ipOrigen = texto;
   }
   if ( puertoOrigen ) {
      *puertoOrigen = ntohs( origen.sin_port );
   }
   return (size_t) st;
}