/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *   SSL Socket class interface (portada a la base idSocket/init de este proyecto)
  *
 **/

#ifndef SSLSocket_hpp
#define SSLSocket_hpp

#include "VSocket.hpp"


class SSLSocket : public VSocket {

   public:
      SSLSocket( bool IPv6 = false );			// Not possible to create with UDP, client constructor
      SSLSocket( char *, char *, bool = false );	// For server connections
      SSLSocket( int );				// envuelve un descriptor ya aceptado (accept())
      ~SSLSocket();
      int Connect( const char *, int );
      int Connect( const char *, const char * );
      size_t Write( const char * );
      size_t Write( const void *, size_t );
      size_t Read( void *, size_t );
      void ShowCerts();
      const char * GetCipher();

      void CopyContext( SSLSocket * original );	// asocia un objeto SSL nuevo usando el mismo contexto que "original"
      SSLSocket * Accept();				// acepta la conexion TCP, copia el contexto del servidor, y hace el handshake SSL
      VSocket * AcceptConnection();			// override del metodo virtual puro de VSocket; delega en Accept()

   private:
      void InitSSL( bool = false );		// Defaults to create a client context, true if server context needed
      void InitContext( bool );
      void LoadCertificates( const char *, const char * );

// Instance variables
      void * Context;				// SSL context
      void * BIO;				// SSL BIO (Basic Input/Output)

};

#endif
