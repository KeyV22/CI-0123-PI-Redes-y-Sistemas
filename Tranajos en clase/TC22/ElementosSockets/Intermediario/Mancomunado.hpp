#ifndef Mancomunado_hpp
#define Mancomunado_hpp

#include <atomic>
#include <ctime>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "PaginasHtml.hpp"
#include "RegistroIslas.hpp"
#include "Socket.hpp"

class ServicioProductosDistribuido;

struct ConfigMancomunado {
   int numeroIsla = 0;              // 1..5 (0 = desactivado)
   std::string ipPrivada;           // IP de la interfaz PRIVADA (la que se anuncia)
   int puertoTcp = 9090;            // mensajes 10, 20, 50, 40 y respuestas
   int puertoUdp = 9090;            // puerto UDP propio (ANNOUNCE / ANNOUNCE_BYE)
   int puertoUdpIslas = 9090;       // puerto UDP al que escuchan las otras islas
   int segundos = 10;               // cada cuanto anuncia y refresca cada isla
   // (id de isla, direccion de broadcast dirigido) de cada OTRA isla
   std::vector<std::pair<std::string, std::string>> destinos;
};

// Comunicacion entre intermediarios de distintas islas segun ProtocoloMancomunadoG02.
// El descubrimiento interno de la isla (servidores de productos) NO pasa por aqui.
class Mancomunado {
   public:
      Mancomunado( ServicioProductosDistribuido & servicio, RegistroIslas & islas, const ConfigMancomunado & config );

      // Bind de UDP y TCP y arranque de hilos: escucha UDP, escucha TCP y un hilo por cada otra isla.
      bool Iniciar();
      // Envia 61 (ANNOUNCE_BYE) a las otras islas.
      void Despedirse();

      // Consultas a otra isla (TCP, un mensaje por conexion)
      bool ConsultarCategorias( const InfoIsla & isla, std::vector<std::string> & categorias ) const;
      bool ConsultarProductos( const InfoIsla & isla, const std::string & categoriaProtocolo, std::vector<FilaProducto> & filas ) const;

      // "Alimentos y bebidas" -> "alimentos_y_bebidas" (formato de categoria/producto del protocolo)
      static std::string NombreProtocolo( const std::string & texto );
      static std::string IdIsla( int numero );                          // 3 -> "INT_03"
      // Busca una IP 172.16.123.x en las interfaces de esta maquina y deduce la isla (octeto / 16)
      static bool DetectarIsla( std::string & ip, int & numeroIsla );
      // Destinos por defecto: broadcast dirigido de las islas 1..6 menos la propia
      static std::vector<std::pair<std::string, std::string>> DestinosPorDefecto( int propia );

   private:
      void HiloUdp();
      void HiloTcp();
      void HiloIsla( std::string id, std::string broadcast );
      void AtenderTcp( Socket cliente );
      std::string Responder( const std::string & mensaje );
      std::string MensajeAnnounce();
      void EnviarAnnounce( const std::string & ip, int puerto );
      bool Pedir( const InfoIsla & isla, const std::string & mensaje, std::string & respuesta ) const;
      bool YaVisto( const std::string & clave );

      ServicioProductosDistribuido & servicio;
      RegistroIslas & islas;
      ConfigMancomunado config;
      std::string miId;
      Socket udp;
      Socket escuchaTcp;
      std::atomic<bool> activo;
      std::atomic<int> contador;
      std::mutex mtxVistos;
      std::map<std::string, std::time_t> vistos;     // id_isla/ID_MSG -> cuando se vio
};

#endif