#ifndef ManejadorConexion_hpp
#define ManejadorConexion_hpp

#include "Socket.hpp"
#include "ServicioProductos.hpp"

// Atiende UNA conexion de cliente de principio a fin: lee el request
// crudo del socket, lo parsea, lo enruta, arma la respuesta HTTP y la
// escribe de vuelta. Pensado para correr dentro de su propio std::thread
// (uno por conexion), lanzado desde Servidor::Ejecutar.
class ManejadorConexion {
   public:
      // servicio se pasa por referencia: Inventario ya tiene su propio
      // mutex interno, asi que es seguro que varios hilos lo usen a la vez.
      // cliente se recibe por valor (move-only): este hilo se vuelve el
      // unico dueño del socket y lo cierra al terminar.
      static void Atender( Socket cliente, ServicioProductos & servicio );

   private:
      // Lee del socket hasta tener el request completo (headers + body
      // segun Content-Length). Devuelve cadena vacia si el cliente cerro
      // la conexion antes de completar el request.
      static std::string LeerRequestCompleto( Socket & cliente );

      // Escribe todos los bytes de 'datos', haciendo varias llamadas a
      // Write() si hace falta (write() puede escribir menos de lo pedido
      // en una sola llamada).
      static void EscribirCompleto( Socket & cliente, const std::string & datos );

      static const size_t TAMANO_BUFFER = 4096;
      static const size_t LIMITE_REQUEST = 1024 * 1024;   // 1 MB, cota de seguridad
      static const int TIMEOUT_LECTURA_SEGUNDOS = 10;     // evita hilos colgados esperando datos
};

#endif