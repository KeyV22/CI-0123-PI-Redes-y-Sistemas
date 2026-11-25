#ifndef Descubrimiento_hpp
#define Descubrimiento_hpp

#include <atomic>
#include <string>
#include <thread>
#include "Socket.hpp"
#include "ClienteRegistro.hpp"
#include <vector>

// Descubrimiento por broadcast UDP:
//  - hilo de escucha (RecvFrom en puertoEscucha): responde BUSCAR_PRODUCTOS
//    con PRODUCTOS|nombre|puertoHttp
//  - hilo de busqueda: mientras no estemos registrados, manda BUSCAR_INTERMEDIARIO
//    al broadcast y fija la direccion del intermediario cuando contesta
class Descubrimiento {
   public:
      Descubrimiento( ClienteRegistro & registro, const std::string & nombre, int puertoHttp, const std::vector<std::string> & broadcasts, int puertoEscucha = 7072, int puertoIntermediario = 7071 );
        ~Descubrimiento();

      void Iniciar( int segundos );
      void Detener();

   private:
      void HiloEscucha();
      void HiloBusqueda( int segundos );
      void Buscar();

      ClienteRegistro & registro;
      std::string nombre;
      int puertoHttp;
      std::vector<std::string> broadcasts;
      int puertoEscucha;
      int puertoIntermediario;

      Socket escucha;
      std::atomic<bool> corriendo;
      std::thread hiloEscucha;
      std::thread hiloBusqueda;
};

#endif