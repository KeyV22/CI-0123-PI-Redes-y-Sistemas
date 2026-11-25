#ifndef Descubrimiento_hpp
#define Descubrimiento_hpp

#include <string>
#include "Socket.hpp"
#include "ServicioProductosDistribuido.hpp"
#include <vector>


class Descubrimiento {
   public:
      Descubrimiento( ServicioProductosDistribuido & servicio, int puertoHttp,const std::vector<std::string> & broadcasts, int puertoUdp = 7071, int puertoProductosUdp = 7072 );
      bool Iniciar( int segundos );

   private:
      void HiloEscucha();
      void HiloBusqueda( int segundos );

      ServicioProductosDistribuido & servicio;
      int puertoHttp;
      std::vector<std::string> broadcasts;     
      int puertoUdp;
      int puertoProductosUdp;
      Socket udp;
      bool soloPrivada = false;   // hay interfaz 172.16.123.x: se ignora lo que llegue de otras redes
};

#endif