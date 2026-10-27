#include <iostream>
#include <stdexcept>
#include <string>
#include "ResponderDescubrimiento.hpp"
#include "Bitacora.hpp"

static const char * SOLICITUD = "DISCOVER/INTERMEDIARIO";

ResponderDescubrimiento::ResponderDescubrimiento( int puertoUDP, int puertoTCP )
   : puertoUDP( puertoUDP ), puertoTCP( puertoTCP ), udp( 'd', false ), activo( false ) {}

ResponderDescubrimiento::~ResponderDescubrimiento() {
   this->Detener();
}

bool ResponderDescubrimiento::Iniciar() {
   try {
      this->udp.Bind( this->puertoUDP );
      this->udp.SetReadTimeout( 1 );   // para que el hilo pueda terminar
   } catch ( const std::exception & e ) {
      std::cerr << "No se pudo iniciar el descubrimiento (UDP " << this->puertoUDP << "): " << e.what() << "\n";
      return false;
   }
   this->activo = true;
   this->hilo = std::thread( &ResponderDescubrimiento::Ciclo, this );
   Bitacora::Registrar( "Descubrimiento: esperando solicitudes UDP en el puerto " + std::to_string( this->puertoUDP ) );
   return true;
}

void ResponderDescubrimiento::Detener() {
   this->activo = false;
   if ( this->hilo.joinable() ) {
      this->hilo.join();
   }
}

void ResponderDescubrimiento::Ciclo() {
   char buffer[256];
   while ( this->activo ) {
      try {
         std::string ipOrigen;
         int puertoOrigen = 0;
         size_t n = this->udp.RecvFrom( buffer, sizeof( buffer ) - 1, &ipOrigen, &puertoOrigen );
         if ( 0 == n ) {
            continue;   // vencio el timeout
         }
         buffer[n] = '\0';
         if ( std::string( buffer ) != SOLICITUD ) {
            Bitacora::Registrar( "Descubrimiento: mensaje ignorado de " + ipOrigen );
            continue;
         }
         std::string respuesta = "OFFER/PRODUCTOS/" + std::to_string( this->puertoTCP );
         this->udp.SendTo( respuesta.c_str(), ipOrigen.c_str(), puertoOrigen );
         Bitacora::Registrar( "Descubrimiento: " + ipOrigen + ":" + std::to_string( puertoOrigen ) + " -> " + respuesta );
      } catch ( const std::exception & e ) {
         Bitacora::Registrar( std::string( "Descubrimiento: error " ) + e.what() );
      }
   }
}