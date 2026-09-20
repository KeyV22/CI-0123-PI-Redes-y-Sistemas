/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *
  *  TicAmazon - Descubrimiento de servidores (ANNOUNCE / DEATH)
  *
  *  Esta implementacion usa
  *  MULTICAST UDP (grupo 239.255.35.1) en vez de broadcast literal,
  *  porque es la opcion mas portable: funciona igual dentro de una misma
  *  maquina (varias instancias en localhost, como en esta prueba), dentro
  *  de una subred /28 real de laboratorio, y no depende de que la interfaz
  *  de red tenga habilitado el broadcast dirigido.
  *
  *  Cada participante (Bodega o Intermediario):
  *    - Escucha el grupo multicast en un hilo aparte, actualizando una
  *      tabla thread-safe de "quien esta vivo" (id -> ip:puerto).
  *    - Al arrancar, manda un ANNOUNCE (60) con su id, ip y puerto real de
  *      servicio (TCP), para que los demas lo agreguen a su tabla.
  *    - Al cerrar (SIGINT o cierre ordenado), manda un DEATH (61) antes de
  *      cerrar sus sockets, para que los demas lo remuevan de su tabla.
  *
 **/

#ifndef DESCUBRIMIENTO_HPP
#define DESCUBRIMIENTO_HPP

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

struct InfoPeer {
   std::string ip;
   int puerto;
};

class Descubrimiento {

   public:
      // puertoMulticast: puerto UDP compartido para el grupo de descubrimiento
      // (todos los participantes que se deben descubrir entre si usan el mismo)
      Descubrimiento( const std::string & miId, const std::string & miIp, int miPuertoServicio, int puertoMulticast );
      ~Descubrimiento();

      void IniciarEscucha();       // arranca el hilo que escucha ANNOUNCE/DEATH de otros
      void AnunciarPresencia();    // manda mi propio ANNOUNCE (60) al grupo
      void AnunciarMuerte();       // manda mi propio DEATH (61) al grupo
      void DetenerEscucha();       // detiene el hilo de escucha (se llama tambien desde el destructor)

      // El mensaje DEATH de este
      // proyecto incluye ip:puerto como campos extra (el documento no los
      // exige, pero tampoco los prohibe) para poder remover solo la
      // instancia exacta que murio, no todas las que comparten el mismo id
      // generico; si llegara un DEATH sin esos campos (formato minimo del
      // documento), se remueven todas las instancias de ese id como
      // respaldo.
      std::map<std::string, std::vector<InfoPeer>> ListarConocidos();   // copia thread-safe de la tabla actual
      std::vector<InfoPeer> BuscarTodos( const std::string & id );      // todas las instancias conocidas de "id"
      bool Buscar( const std::string & id, InfoPeer * infoOut );        // la primera instancia conocida de "id"

   private:
      void TareaEscucha();         // cuerpo del hilo de escucha

      std::string miId_;
      std::string miIp_;
      int miPuertoServicio_;
      int puertoMulticast_;

      int sockEnvio_;              // socket UDP para mandar ANNOUNCE/DEATH
      int sockEscucha_;            // socket UDP (miembro del grupo multicast) para recibir

      std::thread hiloEscucha_;
      std::atomic<bool> corriendo_;

      std::mutex mtxTabla_;
      std::map<std::string, std::vector<InfoPeer>> tabla_;   // id -> [ {ip, puerto}, ... ], protegida por mtxTabla_

};

#endif
