/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  SSL Socket class implementation
  *
  * (Fedora version)
  *
 **/
 
// SSL includes
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <stdexcept>
#include <cstring>		// strlen
#include <cstdio>		// printf
#include <cstdlib>		// free

#include "SSLSocket.h"
#include "Socket.h"

/**
  *  Class constructor
  *     use base class
  *
  *  @param     bool IPv6: if we need a IPv6 socket
  *
  *  Constructor de cliente: crea un socket TCP normal y un contexto/objeto
  *  SSL de tipo cliente, listos para usar en Connect()
  *
 **/
SSLSocket::SSLSocket( bool IPv6 ) {

   this->Init( 's', IPv6 );

   this->Context = nullptr;
   this->BIO = nullptr;

   this->InitSSL();					// Initializes to client context

}


/**
  *  Class constructor
  *     use base class
  *
  *  @param     char * certFileName: archivo con el certificado (modo servidor)
  *  @param     char * keyFileName: archivo con la llave privada (modo servidor)
  *  @param     bool IPv6: if we need a IPv6 socket
  *
  *  Constructor de servidor: crea el socket TCP de escucha, un contexto SSL
  *  de tipo servidor, y carga el certificado/llave que se van a presentar a
  *  los clientes. La conexion "hija" por cliente (post accept()) se maneja
  *  con el constructor SSLSocket( int ).
  *
 **/
SSLSocket::SSLSocket( char * certFileName, char * keyFileName, bool IPv6 ) {

   this->Init( 's', IPv6 );

   this->Context = nullptr;
   this->BIO = nullptr;

   this->InitSSL( true );				// Contexto de servidor
   this->LoadCertificates( certFileName, keyFileName );

}


/**
  *  Class constructor
  *
  *  @param     int id: socket descriptor
  *
  *  Envuelve un descriptor de socket ya aceptado (accept()) del lado del
  *  servidor. NOTA: para completar el handshake de este socket "hijo" se
  *  necesita asociarlo al contexto SSL del servidor (SSL_new + SSL_set_fd +
  *  SSL_accept); eso se completa en la tarea de servidor SSL.
  *
 **/
SSLSocket::SSLSocket( int id ) {

   this->Init( id );

   this->Context = nullptr;
   this->BIO = nullptr;

}


/**
  * Class destructor
  *
 **/
SSLSocket::~SSLSocket() {

// SSL destroy
   if ( nullptr != this->Context ) {
      SSL_CTX_free( reinterpret_cast<SSL_CTX *>( this->Context ) );
   }
   if ( nullptr != this->BIO ) {
      SSL_free( reinterpret_cast<SSL *>( this->BIO ) );
   }

   this->Close();

}


/**
  *  InitSSL
  *     use SSL_new with a defined context
  *
  *  Create a SSL object
  *
  *  @param	bool serverContext: true para crear un objeto SSL de servidor
  *
 **/
void SSLSocket::InitSSL( bool serverContext ) {

   this->InitContext( serverContext );

   SSL * ssl = SSL_new( reinterpret_cast<SSL_CTX *>( this->Context ) );

   if ( nullptr == ssl ) {
      throw std::runtime_error( "SSLSocket::InitSSL( bool )" );
   }

   this->BIO = (void *) ssl;

}


/**
  *  InitContext
  *     use TLS_client_method / TLS_server_method, SSL_CTX_new
  *
  *  Creates a new SSL context to start encrypted comunications, this context is stored in class instance
  *
  *  @param	bool serverContext: true para un contexto de servidor, false para cliente
  *
 **/
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


/**
 *  Load certificates
 *    verify and load certificates
 *
 *  @param	const char * certFileName, file containing certificate
 *  @param	const char * keyFileName, file containing keys
 *
 **/
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
 

/**
 *  Connect
 *     use SSL_connect to establish a secure conection
 *
 *  Create a SSL connection
 *
 *  @param	char * hostName, host name
 *  @param	int port, service number
 *
 **/
int SSLSocket::Connect( const char * hostName, int port ) {
   int st;

   st = this->TryToConnect( hostName, port );		// Establish a non ssl connection first

   SSL_set_fd( reinterpret_cast<SSL *>( this->BIO ), this->sockId );

   st = SSL_connect( reinterpret_cast<SSL *>( this->BIO ) );

   if ( 1 != st ) {
      throw std::runtime_error( "SSLSocket::Connect( const char *, int ), SSL_connect" );
   }

   return st;

}


/**
 *  Connect
 *     use SSL_connect to establish a secure conection
 *
 *  Create a SSL connection
 *
 *  @param	char * hostName, host name
 *  @param	char * service, service name
 *
 **/
int SSLSocket::Connect( const char * host, const char * service ) {
   int st;

   st = this->TryToConnect( host, service );

   SSL_set_fd( reinterpret_cast<SSL *>( this->BIO ), this->sockId );

   st = SSL_connect( reinterpret_cast<SSL *>( this->BIO ) );

   if ( 1 != st ) {
      throw std::runtime_error( "SSLSocket::Connect( const char *, const char * ), SSL_connect" );
   }

   return st;

}


/**
  *  Read
  *     use SSL_read to read data from an encrypted channel
  *
  *  @param	void * buffer to store data read
  *  @param	size_t size, buffer's capacity
  *
  *  @return	size_t byte quantity read
  *
  *  Reads data from secure channel
  *
 **/
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


/**
  *  Write
  *     use SSL_write to write data to an encrypted channel
  *
  *  @param	char * string: texto a escribir (terminado en '\0')
  *
  *  @return	size_t byte quantity written
  *
  *  Writes data to a secure channel
  *
 **/
size_t SSLSocket::Write( const char * string ) {

   return this->Write( (const void *) string, strlen( string ) );

}


/**
  *  Write
  *     use SSL_write to write data to an encrypted channel
  *
  *  @param	const void * buffer to store data to write
  *  @param	size_t size, buffer's capacity
  *
  *  @return	size_t byte quantity written
  *
  *  Writes data to a secure channel
  *
 **/
size_t SSLSocket::Write( const void * buffer, size_t size ) {

   int st = SSL_write( reinterpret_cast<SSL *>( this->BIO ), buffer, (int) size );

   if ( 0 >= st ) {
      throw std::runtime_error( "SSLSocket::Write( void *, size_t )" );
   }

   return (size_t) st;

}


/**
 *   Show SSL certificates
 *
 **/
void SSLSocket::ShowCerts() {
   X509 *cert;
   char *line;

   cert = SSL_get_peer_certificate( reinterpret_cast<SSL *>( this->BIO ) );		 // Get certificates (if available)
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


/**
 *   Return the name of the currently used cipher
 *
 **/
const char * SSLSocket::GetCipher() {

   return SSL_get_cipher( reinterpret_cast<SSL *>( this->BIO ) );

}


/**
  *  CopyContext
  *     Asocia un nuevo objeto SSL (SSL_new) a este socket, usando el MISMO
  *     contexto (certificado/llave ya cargados) que un SSLSocket existente.
  *     Se usa para que cada conexion "hija" aceptada por el servidor
  *     comparta el contexto del socket de escucha, sin tener que volver a
  *     cargar el certificado en cada conexion.
  *
  *  @param	SSLSocket * original: el socket (normalmente el de escucha del
  *		servidor) cuyo contexto se va a reutilizar
  *
 **/
void SSLSocket::CopyContext( SSLSocket * original ) {

   SSL * ssl = SSL_new( reinterpret_cast<SSL_CTX *>( original->Context ) );

   if ( nullptr == ssl ) {
      throw std::runtime_error( "SSLSocket::CopyContext( SSLSocket * )" );
   }

   SSL_set_fd( ssl, this->sockId );

   this->BIO = (void *) ssl;

}


/**
  *  Accept
  *     Version SSL de "AcceptConnection": acepta la conexion TCP entrante
  *     (WaitForConnection, heredado de VSocket), crea el SSLSocket "hijo"
  *     para esa conexion, le copia el contexto del servidor (this), y
  *     completa el handshake SSL (SSL_accept) antes de devolverlo listo
  *     para usar con Read()/Write().
  *
  *  @return	SSLSocket * el socket ya aceptado y con el handshake SSL hecho
  *
 **/
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


/**
  *  AcceptConnection
  *     Override del metodo virtual puro heredado de VSocket. Delega en
  *     Accept(), que hace todo el trabajo real (incluido el handshake SSL).
  *
 **/
VSocket * SSLSocket::AcceptConnection() {

   return this->Accept();

}
