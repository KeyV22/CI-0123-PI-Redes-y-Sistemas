/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  ****** VSocket base class implementation
  *
  * (Fedora version)
  *
 **/

#include <sys/socket.h>
#include <arpa/inet.h>		// ntohs, htons
#include <stdexcept>            // runtime_error
#include <cstring>		// memset
#include <netdb.h>		// getaddrinfo, freeaddrinfo
#include <unistd.h>		// close
/*
#include <cstddef>
#include <cstdio>

//#include <sys/types.h>
*/
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
   type=t;
   this->IPv6=IPv6;
   port=0;
   sockId=-1;

   int domain;
   if(IPv6){
      domain = AF_INET6;
   } else {
      domain = AF_INET; //ipv4
   }

   int sockType;
   if ('s' == t) {
      sockType = SOCK_STREAM; // usa rotocolo [TCP] datos viajan de forma segura, ordenada y sin pérdidas en forma de un flujo continuo de bytes
   } else {
      sockType = SOCK_DGRAM; //d utiliza paquetes independientes llamados datagramas (normalmente con el protocolo UDP)
   }

   sockId=socket(domain, sockType,0); //Esto devuelve un numer

   if ( -1 == this->sockId ) {
      throw std::runtime_error( "VSocket::Init, (reason)" );
   }

}

/*
envuelve un descriptor socket que ya existe. Se usa en modo servidor
para crear un socket "hijo" por cada cliente conectado
*/
void VSocket::Init(int existingSockedId){
   sockId=existingSockedId;
   port=0;
   type='s';

   //Se pregunta al kernel la familia de direcciones de descriptor 
   //Para que IPv6 quede bien sin que el que llama deba indicarlo a pata
   struct sockaddr_storage addr;
   socklen_t addrLen= sizeof(addr);

   if(0==getsockname(existingSockedId,(struct sockaddr *)&addr,&addrLen)){
      this->IPv6=(AF_INET6==addr.ss_family);

   }else{
      this->IPv6=false;
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

int VSocket::Bind(int port){
   int st=-1;

   this->port=port;

   if ( this->IPv6 ) {

      //IPv6
      struct sockaddr_in6 host6;
      memset( (char *) &host6, 0, sizeof( host6 ) );
      host6.sin6_family = AF_INET6;
      host6.sin6_addr = in6addr_any;      // equivalente IPv6 de INADDR_ANY
      host6.sin6_port = htons( port );

      st = bind( this->sockId, (struct sockaddr *) &host6, sizeof( host6 ) );

   } else {

      //IPv4
      struct sockaddr_in host4; //el struct contenia direc ip, puerto y familia
      memset((char *)&host4,0,sizeof(host4));
      host4.sin_family= AF_INET;//Esto era establecer la familia

      //INADDR_ANY = para aceptar conexiones desde Wifi, Ethernet...
      //htonl() = "Host TO Network Long" - convierte el número al formato de red
      //s_addr = La dirección IP en formato binario
      host4.sin_addr.s_addr=htonl(INADDR_ANY); 
      host4.sin_port=htons(port);//Establece el puerto
      memset(host4.sin_zero, '\0',sizeof(host4.sin_zero));//Todo a caracter nulo

      st=bind(this->sockId,(struct sockaddr *)&host4, sizeof(host4));//Adignar direccion y puerto al socket

   }

   if(-1==st){
      throw std::runtime_error("VSocket::Bind,bind");
   }
   return st;
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

   if(this->IPv6){
      struct sockaddr_in6 host6; //contiene información necesaria para representar una dirección IPv6
      memset((char *)&host6,0,sizeof(host6));//Todo en 0 para evitar basura
      host6.sin6_family=AF_INET6; //Direccio ip ivp6

      st=inet_pton(AF_INET6,hostip,&host6.sin6_addr); //convierte una dirección IP en texto a binaria
      if(1!=st){
         throw std::runtime_error( "VSocket::TryToConnect, inet_pton" );
      }

      host6.sin6_port=htons(port); //Convierte el número del puerto al formato de bytes utilizado por la red.
      st=connect(this->sockId,(sockaddr *)&host6,sizeof(host6));
   }else{
      struct sockaddr_in host4;
      memset((char *)&host4,0,sizeof(host4));
      host4.sin_family=AF_INET;

      st=inet_pton(AF_INET,hostip,&host4.sin_addr);
      if(1!=st){
         throw std::runtime_error( "VSocket::TryToConnect, inet_pton" );
      }

      host4.sin_port=htons(port);
      st=connect(this->sockId,(sockaddr *)&host4,sizeof(host4));
   } 
   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::TryToConnect, connect" );
   }

   return st;//0

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
   struct addrinfo hints{};
   struct addrinfo * result=nullptr;
   struct addrinfo * rp=nullptr;

   memset(&hints,0,sizeof(hints)); //Limpiar

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

   st=getaddrinfo(host,service,&hints,&result); //Devuelve las direcciones para conectarse
   if(0!=st){
      throw std::runtime_error( "VSocket::TryToConnect, getaddrinfo" );
   }

   for(rp=result; nullptr!=rp; rp= rp->ai_next){ //addr 1, addr 2, addr3 ..... 
      st= connect(this-> sockId, rp->ai_addr, rp->ai_addrlen);
      if(0==st){ //Si se conecta a alguna es un exito
         break;
      }
   }
   freeaddrinfo(result); //para liberar lo que getaddrinfo reservo

   if(-1==st){
      throw std::runtime_error( "VSocket::TryToConnect,connect" );
   }

   return st;

}




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
   socklen_t addrLen = this->IPv6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);

   ssize_t st=sendto(this->sockId,buffer,size,0,(struct sockaddr *) addr, addrLen); //funcion sendto de UDP

   if(-1==st){
      throw std::runtime_error("VSocket::sendTo(void *,size_t, void *)");
   }

   return (size_t)st;

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
   socklen_t addrLen = this->IPv6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in); //tamaño segun familia

   ssize_t st= recvfrom(this->sockId,buffer,size,0,(struct sockaddr *) addr, &addrLen);//Para recibir datos

   if(-1==st){
      throw std::runtime_error("VSocket::recvFrom(void *, size_t, void *)");
   }

   return (size_t)st;
}

