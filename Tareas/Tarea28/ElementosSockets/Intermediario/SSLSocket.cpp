#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <utility>
#include "SSLSocket.hpp"

SSLSocket::SSLSocket( bool IPv6 ) : Socket( 's', IPv6 ), context( nullptr ), ssl( nullptr ) {
   this->InitContext();
   this->Init();
}

SSLSocket::SSLSocket( const char * certFileName, const char * keyFileName, bool IPv6 )
   : Socket( 's', IPv6 ), context( nullptr ), ssl( nullptr ) {
   this->InitServer( certFileName, keyFileName );
}

SSLSocket::SSLSocket( Socket && conexion )
   : Socket( std::move( conexion ) ), context( nullptr ), ssl( nullptr ) {
}

SSLSocket::~SSLSocket() {
   if ( nullptr != this->ssl ) {
      if ( SSL_is_init_finished( this->ssl ) ) {
         SSL_shutdown( this->ssl );
      }
      SSL_free( this->ssl );
      this->ssl = nullptr;
   }
   if ( nullptr != this->context ) {
      SSL_CTX_free( this->context );
      this->context = nullptr;
   }
}

void SSLSocket::InitContext() {
   this->context = SSL_CTX_new( TLS_client_method() );
   if ( nullptr == this->context ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::InitContext, SSL_CTX_new" );
   }
#ifdef SSL_OP_IGNORE_UNEXPECTED_EOF
   SSL_CTX_set_options( this->context, SSL_OP_IGNORE_UNEXPECTED_EOF );
#endif
}

void SSLSocket::Init() {
   this->ssl = SSL_new( this->context );
   if ( nullptr == this->ssl ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::Init, SSL_new" );
   }
   SSL_set_fd( this->ssl, this->sockId );
}

void SSLSocket::InitServerContext() {
   this->context = SSL_CTX_new( TLS_server_method() );
   if ( nullptr == this->context ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::InitServerContext, SSL_CTX_new" );
   }
#ifdef SSL_OP_IGNORE_UNEXPECTED_EOF
   SSL_CTX_set_options( this->context, SSL_OP_IGNORE_UNEXPECTED_EOF );
#endif
}

void SSLSocket::InitServer( const char * certFileName, const char * keyFileName ) {
   this->InitServerContext();
   this->LoadCertificates( certFileName, keyFileName );
}

void SSLSocket::LoadCertificates( const char * certFileName, const char * keyFileName ) {
   if ( SSL_CTX_use_certificate_file( this->context, certFileName, SSL_FILETYPE_PEM ) <= 0 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::LoadCertificates, use_certificate_file" );
   }
   if ( SSL_CTX_use_PrivateKey_file( this->context, keyFileName, SSL_FILETYPE_PEM ) <= 0 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::LoadCertificates, use_PrivateKey_file" );
   }
   if ( !SSL_CTX_check_private_key( this->context ) ) {
      throw std::runtime_error( "SSLSocket::LoadCertificates, la llave privada no coincide con el certificado" );
   }
}

int SSLSocket::Connect( const char * hostip, int port ) {
   int st = Socket::Connect( hostip, port );
   if ( 1 != SSL_connect( this->ssl ) ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::Connect, SSL_connect" );
   }
   return st;
}

int SSLSocket::Connect( const char * host, const char * service ) {
   int st = Socket::Connect( host, service );
   if ( 1 != SSL_connect( this->ssl ) ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::Connect, SSL_connect" );
   }
   return st;
}

size_t SSLSocket::Read( void * buffer, size_t size ) {
   ERR_clear_error();
   int st = SSL_read( this->ssl, buffer, (int) size );
   if ( st > 0 ) {
      return (size_t) st;
   }
   int err = SSL_get_error( this->ssl, st );
   if ( SSL_ERROR_ZERO_RETURN == err || ( SSL_ERROR_SYSCALL == err && 0 == st ) ) {
      return 0;
   }
   throw std::runtime_error( "SSLSocket::Read, SSL_read" );
}

size_t SSLSocket::Write( const void * buffer, size_t size ) {
   ERR_clear_error();
   int st = SSL_write( this->ssl, buffer, (int) size );
   if ( st <= 0 ) {
      throw std::runtime_error( "SSLSocket::Write, SSL_write" );
   }
   return (size_t) st;
}

size_t SSLSocket::Write( const char * text ) {
   return this->Write( (const void *) text, strlen( text ) );
}

SSLSocket * SSLSocket::AcceptConnection() {
   Socket conexion = Socket::Accept();
   return new SSLSocket( std::move( conexion ) );
}

void SSLSocket::CopyContext( SSLSocket * original ) {
   SSL_CTX_up_ref( original->context );
   this->context = original->context;
   this->ssl = SSL_new( this->context );
   if ( nullptr == this->ssl ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::CopyContext, SSL_new" );
   }
   SSL_set_fd( this->ssl, this->sockId );
}

SSLSocket * SSLSocket::Accept() {
   if ( 1 != SSL_accept( this->ssl ) ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::Accept, SSL_accept" );
   }
   return this;
}

void SSLSocket::ShowCerts() {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
   X509 * cert = SSL_get1_peer_certificate( this->ssl );
#else
   X509 * cert = SSL_get_peer_certificate( this->ssl );
#endif
   if ( nullptr != cert ) {
      char * line = X509_NAME_oneline( X509_get_subject_name( cert ), 0, 0 );
      printf( "Subject: %s\n", line );
      OPENSSL_free( line );
      line = X509_NAME_oneline( X509_get_issuer_name( cert ), 0, 0 );
      printf( "Issuer: %s\n", line );
      OPENSSL_free( line );
      X509_free( cert );
   } else {
      printf( "No certificates.\n" );
   }
}

const char * SSLSocket::GetCipher() {
   return SSL_get_cipher( this->ssl );
}