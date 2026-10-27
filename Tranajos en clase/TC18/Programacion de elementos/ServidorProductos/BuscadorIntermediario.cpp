#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "BuscadorIntermediario.hpp"
#include "Islas.hpp"
#include "Bitacora.hpp"
#include "Socket.hpp"

static const char * SOLICITUD = "DISCOVER/PRODUCTOS";
static const std::string PREFIJO_RESPUESTA = "OFFER/INTERMEDIARIO/";

static int PuertoDeRespuesta( const std::string & mensaje ) {
   if ( mensaje.compare( 0, PREFIJO_RESPUESTA.size(), PREFIJO_RESPUESTA ) != 0 ) {
      return -1;
   }
   std::string numero = mensaje.substr( PREFIJO_RESPUESTA.size() );
   if ( numero.empty() || numero.size() > 5 ||
        numero.find_first_not_of( "0123456789" ) != std::string::npos ) {
      return -1;
   }
   int puerto = std::atoi( numero.c_str() );
   return ( puerto >= 1 && puerto <= 65535 ) ? puerto : -1;
}

BuscadorIntermediario::BuscadorIntermediario( const std::string & broadcast, int puertoUDPIntermediario )
   : broadcast( broadcast ), puertoUDP( puertoUDPIntermediario ), activo( false ) {}

BuscadorIntermediario::~BuscadorIntermediario() {
   this->Detener();
}

void BuscadorIntermediario::Iniciar() {
   this->activo = true;
   this->hilo = std::thread( &BuscadorIntermediario::Ciclo, this );
}

void BuscadorIntermediario::Detener() {
   this->activo = false;
   if ( this->hilo.joinable() ) {
      this->hilo.join();
   }
}

void BuscadorIntermediario::Ciclo() {
   try {
      Socket udp( 'd', false );
      udp.SetBroadcast( true );
      udp.SetReadTimeout( 1 );

      int intento = 0;
      while ( this->activo ) {
         intento++;
         Bitacora::Registrar( "Descubrimiento: buscando intermediario por broadcast (" + this->broadcast + ":"
                              + std::to_string( this->puertoUDP ) + "), intento " + std::to_string( intento ) );
         for ( const std::string & destino : ResolverBroadcast( this->broadcast ) ) {
            try {
               udp.SendTo( SOLICITUD, destino.c_str(), this->puertoUDP );
            } catch ( const std::exception & e ) {
               Bitacora::Registrar( "Descubrimiento: no se pudo enviar a " + destino + ": " + e.what() );
            }
         }

         for ( int espera = 0; espera < 2 && this->activo; espera++ ) {
            char buffer[256];
            std::string ipOrigen;
            int puertoOrigen = 0;
            size_t n = udp.RecvFrom( buffer, sizeof( buffer ) - 1, &ipOrigen, &puertoOrigen );
            if ( 0 == n ) {
               continue;
            }
            buffer[n] = '\0';
            int puertoHTTP = PuertoDeRespuesta( buffer );
            if ( puertoHTTP > 0 ) {
               Bitacora::Registrar( "Descubrimiento: intermediario encontrado en " + ipOrigen + ":" + std::to_string( puertoHTTP ) );
               return;
            }
         }

         for ( int pausa = 0; pausa < 3 && this->activo; pausa++ ) {
            std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
         }
      }
   } catch ( const std::exception & e ) {
      Bitacora::Registrar( std::string( "Descubrimiento: error " ) + e.what() );
   }
}