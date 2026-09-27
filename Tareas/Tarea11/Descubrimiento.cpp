/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *
  *  TicAmazon - Descubrimiento de servidores (implementacion)
  *
 **/

#include "Descubrimiento.hpp"
#include "Protocolo.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <algorithm>

static const char * GRUPO_MULTICAST = "239.255.35.1";


Descubrimiento::Descubrimiento( const std::string & miId, const std::string & miIp, int miPuertoServicio, int puertoMulticast )
   : miId_( miId ), miIp_( miIp ), miPuertoServicio_( miPuertoServicio ), puertoMulticast_( puertoMulticast ),
     sockEnvio_( -1 ), sockEscucha_( -1 ), corriendo_( false ) {

   // --- socket para ENVIAR al grupo multicast ---
   sockEnvio_ = socket( AF_INET, SOCK_DGRAM, 0 );
   if ( -1 == sockEnvio_ ) {
      throw std::runtime_error( "Descubrimiento: socket envio" );
   }

   int ttl = 1;   // no debe salir de la red local
   setsockopt( sockEnvio_, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof( ttl ) );

   // --- socket para RECIBIR del grupo multicast ---
   sockEscucha_ = socket( AF_INET, SOCK_DGRAM, 0 );
   if ( -1 == sockEscucha_ ) {
      throw std::runtime_error( "Descubrimiento: socket escucha" );
   }

   int reuse = 1;
   setsockopt( sockEscucha_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );

   struct sockaddr_in addr;
   memset( &addr, 0, sizeof( addr ) );
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl( INADDR_ANY );
   addr.sin_port = htons( puertoMulticast_ );

   if ( -1 == bind( sockEscucha_, (struct sockaddr *) &addr, sizeof( addr ) ) ) {
      throw std::runtime_error( "Descubrimiento: bind escucha" );
   }

   struct ip_mreq mreq;
   memset( &mreq, 0, sizeof( mreq ) );
   mreq.imr_multiaddr.s_addr = inet_addr( GRUPO_MULTICAST );
   mreq.imr_interface.s_addr = htonl( INADDR_ANY );

   if ( -1 == setsockopt( sockEscucha_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof( mreq ) ) ) {
      throw std::runtime_error( "Descubrimiento: IP_ADD_MEMBERSHIP" );
   }

}


Descubrimiento::~Descubrimiento() {

   DetenerEscucha();

   if ( -1 != sockEnvio_ ) close( sockEnvio_ );
   if ( -1 != sockEscucha_ ) close( sockEscucha_ );

}


void Descubrimiento::IniciarEscucha() {

   corriendo_ = true;
   hiloEscucha_ = std::thread( &Descubrimiento::TareaEscucha, this );

}


void Descubrimiento::DetenerEscucha() {

   if ( corriendo_ ) {
      corriendo_ = false;
      // cerrar el socket de escucha desbloquea el recvfrom() pendiente en el hilo
      shutdown( sockEscucha_, SHUT_RDWR );
      if ( hiloEscucha_.joinable() ) {
         hiloEscucha_.join();
      }
   }

}


void Descubrimiento::TareaEscucha() {

   char buffer[ 512 ];
   struct sockaddr_in remitente;
   socklen_t remitenteLen = sizeof( remitente );

   while ( corriendo_ ) {

      ssize_t n = recvfrom( sockEscucha_, buffer, sizeof( buffer ) - 1, 0,
                             (struct sockaddr *) &remitente, &remitenteLen );

      if ( n <= 0 ) {
         continue;   // el socket se cerro (DetenerEscucha) o hubo un error transitorio
      }

      buffer[ n ] = '\0';
      MensajeV2 m = parsearMensaje( buffer );

      if ( m.origen == miId_ ) {
         continue;   // ignorar mis propios anuncios (el grupo multicast tambien me los devuelve)
      }

      if ( TipoMensaje::ANNOUNCE == m.tipo && m.campos.size() >= 2 ) {

         InfoPeer info;
         info.ip = m.campos[ 0 ];
         info.puerto = std::stoi( m.campos[ 1 ] );

         std::lock_guard<std::mutex> guard( mtxTabla_ );
         auto & instancias = tabla_[ m.origen ];
         bool yaEstaba = false;
         for ( auto & i : instancias ) {
            if ( i.ip == info.ip && i.puerto == info.puerto ) { yaEstaba = true; break; }
         }
         if ( !yaEstaba ) {
            instancias.push_back( info );
         }
         std::cout << "[Descubrimiento] ANNOUNCE recibido de " << m.origen
                   << " (" << info.ip << ":" << info.puerto << "), instancias conocidas de ese id: "
                   << instancias.size() << "\n";

      } else if ( TipoMensaje::DEATH == m.tipo ) {

         std::lock_guard<std::mutex> guard( mtxTabla_ );

         if ( m.campos.size() >= 2 ) {
            // DEATH con ip:puerto (extension propia): remover solo esa instancia exacta
            std::string ipMuerta = m.campos[ 0 ];
            int puertoMuerto = std::stoi( m.campos[ 1 ] );
            auto it = tabla_.find( m.origen );
            if ( tabla_.end() != it ) {
               auto & instancias = it->second;
               instancias.erase(
                  std::remove_if( instancias.begin(), instancias.end(),
                     [&]( const InfoPeer & p ) { return p.ip == ipMuerta && p.puerto == puertoMuerto; } ),
                  instancias.end()
               );
               if ( instancias.empty() ) tabla_.erase( it );
            }
            std::cout << "[Descubrimiento] DEATH recibido de " << m.origen << " (" << ipMuerta << ":" << puertoMuerto
                      << "), instancia removida\n";
         } else {
            // DEATH minimo (solo lo que exige el documento, sin ip:puerto): remueve todas las instancias de ese id
            tabla_.erase( m.origen );
            std::cout << "[Descubrimiento] DEATH recibido de " << m.origen << " (formato minimo), removidas todas sus instancias\n";
         }

      }

   }

}


void Descubrimiento::AnunciarPresencia() {

   std::string msg = construirMensaje( miId_, "ALL", TipoMensaje::ANNOUNCE,
                                        { miIp_, std::to_string( miPuertoServicio_ ) } );

   struct sockaddr_in destino;
   memset( &destino, 0, sizeof( destino ) );
   destino.sin_family = AF_INET;
   destino.sin_port = htons( puertoMulticast_ );
   destino.sin_addr.s_addr = inet_addr( GRUPO_MULTICAST );

   sendto( sockEnvio_, msg.c_str(), msg.size(), 0, (struct sockaddr *) &destino, sizeof( destino ) );

}


void Descubrimiento::AnunciarMuerte() {

   // Se incluyen ip y puerto como campos extra (el documento no los exige
   // para DEATH, pero tampoco los prohibe). Sin esto, como el documento no
   // numera los servidores de bodega (todos son "SERV"), un DEATH de una
   // instancia borraria de la tabla a TODAS las instancias con ese mismo id
   // generico, incluidas las que siguen vivas.
   std::string msg = construirMensaje( miId_, "ALL", TipoMensaje::DEATH,
                                        { miIp_, std::to_string( miPuertoServicio_ ) } );

   struct sockaddr_in destino;
   memset( &destino, 0, sizeof( destino ) );
   destino.sin_family = AF_INET;
   destino.sin_port = htons( puertoMulticast_ );
   destino.sin_addr.s_addr = inet_addr( GRUPO_MULTICAST );

   sendto( sockEnvio_, msg.c_str(), msg.size(), 0, (struct sockaddr *) &destino, sizeof( destino ) );

}


std::map<std::string, std::vector<InfoPeer>> Descubrimiento::ListarConocidos() {

   std::lock_guard<std::mutex> guard( mtxTabla_ );
   return tabla_;   // copia

}


std::vector<InfoPeer> Descubrimiento::BuscarTodos( const std::string & id ) {

   std::lock_guard<std::mutex> guard( mtxTabla_ );
   auto it = tabla_.find( id );
   if ( tabla_.end() == it ) {
      return {};
   }
   return it->second;   // copia

}


bool Descubrimiento::Buscar( const std::string & id, InfoPeer * infoOut ) {

   std::lock_guard<std::mutex> guard( mtxTabla_ );
   auto it = tabla_.find( id );
   if ( tabla_.end() == it || it->second.empty() ) {
      return false;
   }
   *infoOut = it->second[ 0 ];   // la primera instancia conocida
   return true;

}
