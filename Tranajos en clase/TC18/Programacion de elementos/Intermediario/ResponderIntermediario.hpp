#ifndef ResponderIntermediario_hpp
#define ResponderIntermediario_hpp

#include <atomic>
#include <thread>
#include "Socket.hpp"

// Servidor -> broadcast:        DISCOVER/PRODUCTOS
// Intermediario -> servidor:    OFFER/INTERMEDIARIO/<puertoHTTP>   (unicast)
class ResponderIntermediario {
   public:
      ResponderIntermediario( int puertoUDP, int puertoHTTP );
      ~ResponderIntermediario();

      bool Iniciar();
      void Detener();

   private:
      void Ciclo();

      int puertoUDP;
      int puertoHTTP;
      Socket udp;
      std::atomic<bool> activo;
      std::thread hilo;
};

#endif