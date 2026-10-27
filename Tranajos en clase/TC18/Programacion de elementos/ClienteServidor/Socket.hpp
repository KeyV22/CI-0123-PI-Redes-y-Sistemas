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
#include <string>

class Socket : public VSocket {

   public:
      Socket( char, bool = false );
      ~Socket();
      int Connect( const char *, int );
      int Connect( const char *, const char * );
      size_t Read( void *, size_t );
      size_t Write( const void *, size_t );
      size_t Write( const char * );

      // --- Extension para datagramas UDP (unicast o broadcast) ---
      // Envia un datagrama a ip:puerto sin necesidad de connect().
      size_t SendTo( const void *, size_t, const char * ip, int puerto );
      size_t SendTo( const char * texto, const char * ip, int puerto );
      // Espera un datagrama. Devuelve los bytes recibidos y, si se piden,
      // la IP y el puerto de quien lo envio. Si se configuro
      // SetReadTimeout() y vence, devuelve 0 en vez de lanzar excepcion.
      size_t RecvFrom( void *, size_t, std::string * ipOrigen = nullptr, int * puertoOrigen = nullptr );


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

   protected:

   private:
      Socket( int fdExistente, bool IPv6 );   // usado por Accept()
};

#endif