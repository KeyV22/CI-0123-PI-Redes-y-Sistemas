#ifndef SSLSocket_hpp
#define SSLSocket_hpp

#include <openssl/ssl.h>
#include <openssl/err.h>
#include "Socket.hpp"

class SSLSocket : public Socket {

   public:
      SSLSocket( bool IPv6 = false );
      SSLSocket( const char * certFileName, const char * keyFileName, bool IPv6 = false );
      SSLSocket( Socket && conexion );
      ~SSLSocket();

      int Connect( const char *, int );
      int Connect( const char *, const char * );

      size_t Read( void *, size_t );
      size_t Write( const void *, size_t );
      size_t Write( const char * );

      SSLSocket * AcceptConnection();
      SSLSocket * Accept();
      void CopyContext( SSLSocket * original );
      void ShowCerts();
      const char * GetCipher();

   protected:
      void InitContext();
      void Init();
      void InitServerContext();
      void InitServer( const char * certFileName, const char * keyFileName );
      void LoadCertificates( const char * certFileName, const char * keyFileName );

      SSL_CTX * context;
      SSL * ssl;
};

#endif