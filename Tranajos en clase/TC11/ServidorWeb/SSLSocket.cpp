/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  SSL Socket class implementation (portada a la base idSocket/init de este proyecto)
  *
 **/

// SSL includes
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <stdexcept>
#include <cstring>		// strlen
#include <cstdio>		// printf
#include <cstdlib>		// free
#include <sys/socket.h>		// getsockname, sockaddr_storage
#include <netinet/in.h>		// AF_INET6

#include "SSLSocket.hpp"
#include "Socket.hpp"

/**
  *  Class constructor (cliente): crea un socket TCP normal y un
  *  contexto/objeto SSL de tipo cliente, listos para usar en Connect()
  *
 **/
SSLSocket::SSLSocket( bool IPv6 ) {

   this->init( 's', IPv6 );

   this->Context = nullptr;
   this->BIO = nullptr;

   this->InitSSL();				// Initializes to client context

}


/**
  *  Class constructor (servidor): crea el socket TCP de escucha, un
  *  contexto SSL de tipo servidor, y carga el certificado/llave.
  *
 **/
SSLSocket::SSLSocket( char * certFileName, char * keyFileName, bool IPv6 ) {

   this->init( 's', IPv6 );

   this->Context = nullptr;
   this->BIO = nullptr;

   this->InitSSL( true );			// Contexto de servidor
   this->LoadCertificates( certFileName, keyFileName );

}


/**
  *  Class constructor
  *     Envuelve un descriptor de socket ya aceptado (accept()) del lado del
  *     servidor. Esta base de VSocket no tiene un "init(int)", asi que se
  *     inicializan aqui directamente los campos protegidos heredados
  *     (idSocket/port/type/IPv6), igual que hace Socket::Socket(int) en
  *     esta misma base.
  *
 **/
SSLSocket::SSLSocket( int id ) {

   this->idSocket = id;
   this->port = 0;
   this->type = 's';

   struct sockaddr_storage addr;
   socklen_t addrLen = sizeof( addr );
   if ( 0 == getsockname( id, (struct sockaddr *) &addr, &addrLen ) ) {
      this->IPv6 = ( AF_INET6 == addr.ss_family );
   } else {
      this->IPv6 = false;
   }

   this->Context = nullptr;
   this->BIO = nullptr;

}


/**
  * Class destructor
  *
 **/
SSLSocket::~SSLSocket() {

   if ( nullptr != this->Context ) {
      SSL_CTX_free( reinterpret_cast<SSL_CTX *>( this->Context ) );
   }
   if ( nullptr != this->BIO ) {
      SSL_free( reinterpret_cast<SSL *>( this->BIO ) );
   }

   this->Close();

}


void SSLSocket::InitSSL( bool serverContext ) {

   this->InitContext( serverContext );

   SSL * ssl = SSL_new( reinterpret_cast<SSL_CTX *>( this->Context ) );

   if ( nullptr == ssl ) {
      throw std::runtime_error( "SSLSocket::InitSSL( bool )" );
   }

   this->BIO = (void *) ssl;

}


void SSLSocket::InitContext( bool serverContext ) {
   const SSL_METHOD * method;
   SSL_CTX * context;

   if ( serverContext ) {
      method = TLS_server_method();
   } else {
      method = TLS_client_method();
   }

   if ( nullptr == method ) {
      throw std::runtime_error( "SSLSocket::InitContext( bool ), method" );
   }

   context = SSL_CTX_new( method );

   if ( nullptr == context ) {
      throw std::runtime_error( "SSLSocket::InitContext( bool ), SSL_CTX_new" );
   }

   this->Context = (void *) context;

}


void SSLSocket::LoadCertificates( const char * certFileName, const char * keyFileName ) {

   SSL_CTX * context = reinterpret_cast<SSL_CTX *>( this->Context );

   if ( 0 >= SSL_CTX_use_certificate_file( context, certFileName, SSL_FILETYPE_PEM ) ) {
      throw std::runtime_error( "SSLSocket::LoadCertificates, SSL_CTX_use_certificate_file" );
   }

   if ( 0 >= SSL_CTX_use_PrivateKey_file( context, keyFileName, SSL_FILETYPE_PEM ) ) {
      throw std::runtime_error( "SSLSocket::LoadCertificates, SSL_CTX_use_PrivateKey_file" );
   }

   if ( 0 == SSL_CTX_check_private_key( context ) ) {
      throw std::runtime_error( "SSLSocket::LoadCertificates, SSL_CTX_check_private_key" );
   }

}


int SSLSocket::Connect( const char * hostName, int port ) {
   int st;

   st = this->TryToConnect( hostName, port );

   SSL_set_fd( reinterpret_cast<SSL *>( this->BIO ), this->idSocket );

   st = SSL_connect( reinterpret_cast<SSL *>( this->BIO ) );

   if ( 1 != st ) {
      throw std::runtime_error( "SSLSocket::Connect( const char *, int ), SSL_connect" );
   }

   return st;

}


int SSLSocket::Connect( const char * host, const char * service ) {
   int st;

   st = this->TryToConnect( host, service );

   SSL_set_fd( reinterpret_cast<SSL *>( this->BIO ), this->idSocket );

   st = SSL_connect( reinterpret_cast<SSL *>( this->BIO ) );

   if ( 1 != st ) {
      throw std::runtime_error( "SSLSocket::Connect( const char *, const char * ), SSL_connect" );
   }

   return st;

}


size_t SSLSocket::Read( void * buffer, size_t size ) {

   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );
   int st = SSL_read( ssl, buffer, (int) size );

   if ( 0 >= st ) {

      int sslErr = SSL_get_error( ssl, st );

      // Cierre normal de la conexion (close_notify, o el peer cerro el TCP sin avisar):
      // esto es EOF, no un error -- debe devolver 0 para que "while(Read()>0)" termine bien
      if ( SSL_ERROR_ZERO_RETURN == sslErr || SSL_ERROR_SYSCALL == sslErr ) {
         return 0;
      }

      throw std::runtime_error( "SSLSocket::Read( void *, size_t )" );
   }

   return (size_t) st;

}


size_t SSLSocket::Write( const char * string ) {

   return this->Write( (const void *) string, strlen( string ) );

}


size_t SSLSocket::Write( const void * buffer, size_t size ) {

   int st = SSL_write( reinterpret_cast<SSL *>( this->BIO ), buffer, (int) size );

   if ( 0 >= st ) {
      throw std::runtime_error( "SSLSocket::Write( void *, size_t )" );
   }

   return (size_t) st;

}


void SSLSocket::ShowCerts() {
   X509 *cert;
   char *line;

   cert = SSL_get_peer_certificate( reinterpret_cast<SSL *>( this->BIO ) );
   if ( nullptr != cert ) {
      printf("Server certificates:\n");
      line = X509_NAME_oneline( X509_get_subject_name( cert ), 0, 0 );
      printf( "Subject: %s\n", line );
      free( line );
      line = X509_NAME_oneline( X509_get_issuer_name( cert ), 0, 0 );
      printf( "Issuer: %s\n", line );
      free( line );
      X509_free( cert );
   } else {
      printf( "No certificates.\n" );
   }

}


const char * SSLSocket::GetCipher() {

   return SSL_get_cipher( reinterpret_cast<SSL *>( this->BIO ) );

}


void SSLSocket::CopyContext( SSLSocket * original ) {

   SSL * ssl = SSL_new( reinterpret_cast<SSL_CTX *>( original->Context ) );

   if ( nullptr == ssl ) {
      throw std::runtime_error( "SSLSocket::CopyContext( SSLSocket * )" );
   }

   SSL_set_fd( ssl, this->idSocket );

   this->BIO = (void *) ssl;

}


SSLSocket * SSLSocket::Accept() {

   int idCliente = this->WaitForConnection();		// accept() TCP normal (heredado de VSocket)

   SSLSocket * cliente = new SSLSocket( idCliente );

   cliente->CopyContext( this );			// mismo contexto (certificado/llave) que el servidor

   int st = SSL_accept( reinterpret_cast<SSL *>( cliente->BIO ) );

   if ( 1 != st ) {
      delete cliente;
      throw std::runtime_error( "SSLSocket::Accept, SSL_accept" );
   }

   return cliente;

}


VSocket * SSLSocket::AcceptConnection() {

   return this->Accept();

}
