/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  ******   VSocket base class interface
  *
  * (Fedora version)
  *
 **/

#ifndef VSocket_h
#define VSocket_h
#include <cstddef>
#include <string>
 
class VSocket {
   public:
       void Init( char, bool = false );
      ~VSocket();

      void Close();
      int TryToConnect( const char *, int );
      int TryToConnect( const char *, const char * );
      int TryToBind( int port );

      // Util para no dejar hilos colgados esperando datos que nunca
      // llegan.
      void SetReadTimeout( int segundos );

      // Activa o desactiva SO_BROADCAST: sin esto el SO rechaza (EACCES)
      // cualquier sendto() hacia una direccion de broadcast.
      void SetBroadcast( bool activar = true );

      // Calcula la direccion de broadcast dirigido de una red IPv4.
      // Ejemplo: DireccionBroadcast( "172.16.123.48", 28 ) -> "172.16.123.63"
      static std::string DireccionBroadcast( const char * ip, int prefijo );

      virtual int Connect( const char *, int ) = 0;
      virtual int Connect( const char *, const char * ) = 0;

      virtual size_t Read( void *, size_t ) = 0;
      virtual size_t Write( const void *, size_t ) = 0;
      virtual size_t Write( const char * ) = 0;

   protected:
      int sockId;   // Socket identifier
      bool IPv6;      // Is IPv6 socket?
      int port;       // Socket associated port
      char type;      // Socket type (datagram, stream, etc.)
        
};

#endif // VSocket_h