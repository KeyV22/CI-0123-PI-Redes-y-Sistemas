#ifndef BuscadorIntermediario_hpp
#define BuscadorIntermediario_hpp

#include <atomic>
#include <string>
#include <thread>

// Manda (SendTo) DISCOVER/PRODUCTOS por broadcast y espera (RecvFrom)
// OFFER/INTERMEDIARIO/<puertoHTTP>. Repite hasta encontrarlo.
class BuscadorIntermediario {
   public:
      BuscadorIntermediario( const std::string & broadcast, int puertoUDPIntermediario );
      ~BuscadorIntermediario();

      void Iniciar();
      void Detener();

   private:
      void Ciclo();

      std::string broadcast;
      int puertoUDP;
      std::atomic<bool> activo;
      std::thread hilo;
};

#endif