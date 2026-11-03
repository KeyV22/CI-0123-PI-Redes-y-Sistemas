#ifndef ClienteRegistro_hpp
#define ClienteRegistro_hpp

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include "Inventario.hpp"


class ClienteRegistro {
   public:
      ClienteRegistro( Inventario & inventario, const std::string & nombre, int miPuerto );
      ~ClienteRegistro();

      void FijarIntermediario( const std::string & host, int puerto );
      bool Conectado() const { return conectado; }   // ultimo registro exitoso

      void Iniciar( int segundos );
      void Detener();

   private:
      bool EnviarRegistro();

      Inventario & inventario;
      std::string nombre;
      int miPuerto;

      mutable std::mutex mtx;            // protege host/puerto del intermediario
      std::string hostIntermediario;
      int puertoIntermediario;

      std::atomic<bool> conectado;
      std::atomic<bool> corriendo;
      std::thread hilo;
};

#endif