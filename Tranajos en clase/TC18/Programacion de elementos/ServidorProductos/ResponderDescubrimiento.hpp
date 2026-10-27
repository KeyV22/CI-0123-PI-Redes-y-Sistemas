#ifndef ResponderDescubrimiento_hpp
#define ResponderDescubrimiento_hpp

#include <atomic>
#include <thread>
#include "Socket.hpp"

// Hilo del servidor de productos que espera (RecvFrom) las solicitudes de
// descubrimiento que los intermediarios mandan (SendTo) a la direccion de
// broadcast de la isla.
//   Intermediario -> broadcast:   DISCOVER/INTERMEDIARIO
//   Servidor -> intermediario:    OFFER/PRODUCTOS/<puertoTCP>     (unicast)
class ResponderDescubrimiento {
   public:
      ResponderDescubrimiento( int puertoUDP, int puertoTCP );
      ~ResponderDescubrimiento();

      bool Iniciar();   // Bind() del puerto UDP y lanza el hilo
      void Detener();   // pide al hilo que termine y espera (maximo ~1 s)

   private:
      void Ciclo();

      int puertoUDP;
      int puertoTCP;
      Socket udp;
      std::atomic<bool> activo;
      std::thread hilo;
};

#endif