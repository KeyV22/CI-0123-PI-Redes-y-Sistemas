#ifndef DescubridorServidor_hpp
#define DescubridorServidor_hpp

#include <string>

struct ServidorEncontrado {
   std::string ip;
   int puerto;   // puerto TCP (HTTP) del servidor de productos
};

// Manda (SendTo) DISCOVER/INTERMEDIARIO a la direccion de broadcast de la isla
// y espera (RecvFrom) OFFER/PRODUCTOS/<puertoTCP>.
class DescubridorServidor {
   public:
      // broadcast: ej. "172.16.123.63" (isla 3)   puertoUDP: ej. 9090
      // intentos: veces que repite si nadie responde   timeoutSeg: espera por intento
      static bool Buscar( const std::string & broadcast, int puertoUDP,
                          int intentos, int timeoutSeg, ServidorEncontrado & resultado );
};

#endif