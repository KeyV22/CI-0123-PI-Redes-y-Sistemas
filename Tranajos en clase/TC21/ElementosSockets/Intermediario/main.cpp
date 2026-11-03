#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include "ServicioProductosDistribuido.hpp"
#include "Servidor.hpp"
#include "Descubrimiento.hpp"
#include "Broadcasts.hpp"
#include "RegistroIslas.hpp"
#include "Mancomunado.hpp"
#include <csignal>

static std::atomic<bool> terminar( false );
static void AlRecibirSenal( int ) { terminar = true; }

// Uso: ./intermediario [puerto] [broadcast] [host:puerto ...] [opciones clave=valor]
//   Un argumento SIN ':' ni '=' es la direccion de broadcast de la isla (servidores de productos);
//   con ':' es una semilla (servidor de productos conocido).
//
// Opciones de la comunicacion ENTRE ISLAS (ProtocoloMancomunadoG02). Todas son opcionales:
// por defecto la isla y la IP privada se deducen de la interfaz 172.16.123.x.
//   isla=3            numero de esta isla (1..6)
//   ip=172.16.123.52  IP de la interfaz PRIVADA que se anuncia a las otras islas
//   tcp=9090          puerto TCP del protocolo mancomunado
//   udp=9090          puerto UDP propio para ANNOUNCE / ANNOUNCE_BYE
//   udpislas=9090     puerto UDP al que escuchan las otras islas
//   bc=A.B.C.D        usar esta direccion en vez del broadcast dirigido de cada isla (pruebas)
//   seg=10            segundos entre anuncios
int main( int argc, char * argv[] ) {
   signal( SIGPIPE, SIG_IGN );
   signal( SIGINT, AlRecibirSenal );
   signal( SIGTERM, AlRecibirSenal );

   int puerto = ( argc > 1 ) ? std::atoi( argv[1] ) : 8080;
   std::string broadcast;
   ConfigMancomunado cfg;
   std::string bcPrueba;

   ServicioProductosDistribuido servicio;
   for ( int i = 2; i < argc; i++ ) {
      std::string arg = argv[i];
      size_t eq = arg.find( '=' );
      if ( eq != std::string::npos ) {
         std::string clave = arg.substr( 0, eq ), valor = arg.substr( eq + 1 );
         if ( clave == "isla" ) cfg.numeroIsla = std::atoi( valor.c_str() );
         else if ( clave == "ip" ) cfg.ipPrivada = valor;
         else if ( clave == "tcp" ) cfg.puertoTcp = std::atoi( valor.c_str() );
         else if ( clave == "udp" ) { cfg.puertoUdp = std::atoi( valor.c_str() ); cfg.puertoUdpIslas = cfg.puertoUdp; }
         else if ( clave == "udpislas" ) cfg.puertoUdpIslas = std::atoi( valor.c_str() );
         else if ( clave == "bc" ) bcPrueba = valor;
         else if ( clave == "seg" ) cfg.segundos = std::atoi( valor.c_str() );
         else { std::cerr << "Opcion desconocida: " << arg << "\n"; return 1; }
         continue;
      }
      size_t dp = arg.rfind( ':' );
      if ( dp == std::string::npos ) {
         broadcast = arg;
         continue;
      }
      int p = std::atoi( arg.substr( dp + 1 ).c_str() );
      if ( p <= 0 ) {
         std::cerr << "Argumento invalido (use host:puerto): " << arg << "\n";
         return 1;
      }
      servicio.AgregarSemilla( arg.substr( 0, dp ), p );
   }

   Servidor servidor( puerto );
   if ( !servidor.Iniciar() ) {
      return 1;
   }
   servicio.IniciarMonitor( 10 );

   Descubrimiento descubrimiento( servicio, puerto, ResolverBroadcasts( broadcast ) );
   descubrimiento.Iniciar( 10 );

   // ---- Comunicacion con los intermediarios de las otras islas
   if ( cfg.numeroIsla == 0 || cfg.ipPrivada.empty() ) {
      std::string ip;
      int n = 0;
      if ( Mancomunado::DetectarIsla( ip, n ) ) {
         if ( cfg.ipPrivada.empty() ) cfg.ipPrivada = ip;
         if ( cfg.numeroIsla == 0 ) cfg.numeroIsla = n;
      }
   }
   RegistroIslas islas;
   Mancomunado * mancomunado = nullptr;
   if ( cfg.numeroIsla >= 1 && cfg.numeroIsla <= 6 && !cfg.ipPrivada.empty() ) {
      cfg.destinos = Mancomunado::DestinosPorDefecto( cfg.numeroIsla );
      if ( !bcPrueba.empty() ) {
         for ( auto & d : cfg.destinos ) d.second = bcPrueba;
      }
      mancomunado = new Mancomunado( servicio, islas, cfg );
      if ( mancomunado->Iniciar() ) {
         servicio.FijarMancomunado( mancomunado, &islas );
      } else {
         delete mancomunado;
         mancomunado = nullptr;
      }
   } else {
      std::cout << "Aviso: sin conexion con otras islas (no se encontro una IP 172.16.123.x; use isla=N ip=A.B.C.D)\n";
   }

   // Al cerrar con Ctrl+C se avisa a las otras islas (61) antes de salir
   std::thread( [mancomunado]() {
      while ( !terminar ) std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
      if ( mancomunado != nullptr ) mancomunado->Despedirse();
      std::_Exit( 0 );
   } ).detach();

   servidor.Ejecutar( servicio );
   return 0;
}