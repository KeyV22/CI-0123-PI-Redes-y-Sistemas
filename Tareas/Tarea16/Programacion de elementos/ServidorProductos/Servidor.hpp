#ifndef Servidor_hpp
#define Servidor_hpp

#include <atomic>
#include "Socket.hpp"
#include "ServicioProductos.hpp"

// Servidor TCP: hace bind/listen sobre un puerto y por cada conexion
// aceptada lanza un hilo que la atiende de forma
// independiente.
class Servidor {
   public:
      explicit Servidor( int puerto );

      // Hace Bind() y Listen() sobre el socket de escucha. Devuelve
      // false si algo falla (puerto ocupado, permisos, etc.).
      bool Iniciar();

      // Loop principal: Accept() en bucle, un hilo por conexion.
      // Bloqueante -- se corre en el hilo principal de main().
      void Ejecutar( ServicioProductos & servicio );

      // Permite pedirle al servidor que termine el loop (ej. para pruebas).
      void Detener();

      // Pide que el servidor termine: marca la bandera y se conecta a si
      // mismo para despertar el Accept() bloqueado. Se puede llamar desde
      // cualquier hilo.
      static void SolicitarApagado();
   private:
      int puerto;
      Socket escucha;             // socket de tipo 's' (stream/TCP)
      std::atomic<bool> corriendo;
      static std::atomic<bool> apagar;
      static int puertoEscucha;
};

#endif