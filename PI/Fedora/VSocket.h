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

#include <cstddef> //size_t
 
class VSocket {
   public:

      // Inicializa el socket.
      // Recibe:
      //     char
      //        tipo de socket.
      //     bool = false
      //         Indica si se quiere utilizar IPv6.
      //
      //     socket.Init('s'); automáticamente IPv6 será false.
      //
      //     socket.Init('s', true); utilizaremos IPv6.
      void Init( char, bool = false );
      ~VSocket();

      //Para cerrar el socket
      void Close();

      // Esta función es para intentar establecer una conexión.
      // Recibe:
      //     const char *
      //         Un puntero a caracteres.
      //         que va a representar una dirección o nombre de host.
      //     int
      //         El número de puerto.
      int TryToConnect( const char *, int );
      int TryToConnect( const char *, const char * );

      // Esta función va a ser redefinida por una clase hija.
      // "= 0" esto significaba VIRTUAL PURA.
      // Recibe:
      //     const char *
      //         Dirección/host.
      //     int
      //         Puerto.
      virtual int Connect( const char *, int ) = 0;
      virtual int Connect( const char *, const char * ) = 0;

      // LEER datos desde el socket.
      // Recibe:
      //     void *
      //         Un puntero genérico al espacio donde se almacenarán los datos recibidos.
      //     size_t
      //         Cantidad/tamaño de datos que se quieren leer.
      // Devuelve: size_t
      //         Cantidad de datos leídos.
      virtual size_t Read( void *, size_t ) = 0;

      // para ESCRIBIR/ENVIAR datos.
      // Devuelve la cantidad de datos escritos/enviados.
      virtual size_t Write( const void *, size_t ) = 0;
      virtual size_t Write( const char * ) = 0;

   protected:
      int sockId;   // Socket identifier
      bool IPv6;      // Is IPv6 socket?
      int port;       // Socket associated port
      char type;      // Socket type (datagram, stream, etc.)
        
};

#endif // VSocket_h
