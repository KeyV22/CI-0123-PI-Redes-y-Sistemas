/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  ****** Socket derived class interface
  *
  * (Fedora version)
  *
 **/

#ifndef Socket_h
#define Socket_h
#include "Vsocket.hpp"

class Socket : public VSocket {

   public:
      Socket( char, bool = false );
      ~Socket();
      int Connect( const char *, int );
      int Connect( const char *, const char * );
      size_t Read( void *, size_t );
      size_t Write( const void *, size_t );
      size_t Write( const char * );

      // --- Extension para funcionalidad de SERVIDOR ---
      int Bind( int port );
      int Listen( int backlog );
      Socket Accept();            // bloquea hasta que llegue una conexion

      // Un Socket es dueno de un file descriptor: copiarlo dejaria dos
      // objetos creyendo que son dueños del mismo descriptor, y ambos
      // destructores intentarian cerrarlo (doble-close / UB). Por eso
      // se borra la copia y se agrega semantica de movimiento, para
      // poder devolver el Socket de Accept() por valor y pasarlo a un
      // std::thread con std::move.
      Socket( const Socket & ) = delete;
      Socket & operator=( const Socket & ) = delete;
      Socket( Socket && otro ) noexcept;
      Socket & operator=( Socket && otro ) noexcept;
      
      
      void HabilitarBroadcast();
      bool SendTo( const void * datos, size_t largo, const std::string & ip, int puerto );
      bool SendTo( const std::string & texto, const std::string & ip, int puerto );
      // Devuelve bytes recibidos, o -1 si hubo timeout/error (no lanza excepcion)
      long RecvFrom( void * buffer, size_t size, std::string & ipOrigen, int & puertoOrigen );
      // IP local con la que quedo este socket (util despues de Connect)
      std::string IpLocal();

   protected:

   private:
      Socket( int fdExistente, bool IPv6 );   // usado por Accept()
};

#endif