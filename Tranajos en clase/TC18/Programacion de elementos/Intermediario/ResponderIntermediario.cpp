#include <iostream>
#include <stdexcept>
#include <string>
#include "ResponderIntermediario.hpp"
#include "Bitacora.hpp"

static const char * SOLICITUD = "DISCOVER/PRODUCTOS";

ResponderIntermediario::ResponderIntermediario( int puertoUDP, int puertoHTTP )
   : puertoUDP( puertoUDP ), puertoHTTP( puertoHTTP ), udp( 'd', false ), activo( false ) {}

ResponderIntermediario::~ResponderIntermediario() {
   this->Detener();
}

bool ResponderIntermediario::Iniciar() {
   try {
      this->udp.Bind( this->puertoUDP );
      this->udp.SetReadTimeout( 1 );
   } catch ( const std::exception & e ) {
      std::cerr << "No se pudo iniciar el descubrimiento (UDP " << this->puertoUDP << "): " << e.what() << "\n";
      return false;
   }
   this->activo = true;
   this->hilo = std::thread( &ResponderIntermediario::Ciclo, this );
   Bitacora::Registrar( "Descubrimiento: esperando servidores en el puerto UDP " + std::to_string( this->puertoUDP ) );
   return true;
}

void ResponderIntermediario::Detener() {
   this->activo = false;
   if ( this->hilo.joinable() ) {
      this->hilo.join();
   }
}

void ResponderIntermediario::Ciclo() {
   char buffer[256];
   while ( this->activo ) {
      try {
         std::string ipOrigen;
         int puertoOrigen = 0;
         size_t n = this->udp.RecvFrom( buffer, sizeof( buffer ) - 1, &ipOrigen, &puertoOrigen );
         if ( 0 == n ) {
            continue;
         }
         buffer[n] = '\0';
         if ( std::string( buffer ) != SOLICITUD ) {
            Bitacora::Registrar( "Descubrimiento: mensaje ignorado de " + ipOrigen );
            continue;
         }
         std::string respuesta = "OFFER/INTERMEDIARIO/" + std::to_string( this->puertoHTTP );
         this->udp.SendTo( respuesta.c_str(), ipOrigen.c_str(), puertoOrigen );
         Bitacora::Registrar( "Descubrimiento: servidor " + ipOrigen + ":" + std::to_string( puertoOrigen ) + " -> " + respuesta );
      } catch ( const std::exception & e ) {
         Bitacora::Registrar( std::string( "Descubrimiento: error " ) + e.what() );
      }
   }
}