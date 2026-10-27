#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "DescubridorServidor.hpp"
#include "Socket.hpp"

static const char * SOLICITUD  = "DISCOVER/INTERMEDIARIO";
static const std::string PREFIJO_RESPUESTA = "OFFER/PRODUCTOS/";

// Devuelve el puerto TCP anunciado o -1 si la respuesta no tiene el formato esperado.
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

bool DescubridorServidor::Buscar( const std::string & broadcast, int puertoUDP,
                                  int intentos, int timeoutSeg, ServidorEncontrado & resultado ) {
   try {
      Socket udp( 'd', false );
      udp.SetBroadcast( true );
      udp.SetReadTimeout( timeoutSeg );

      for ( int intento = 1; intento <= intentos; intento++ ) {
         std::cout << "Buscando servidor de productos por broadcast (" << broadcast << ":"
                   << puertoUDP << "), intento " << intento << "/" << intentos << "\n";
         udp.SendTo( SOLICITUD, broadcast.c_str(), puertoUDP );

         auto limite = std::chrono::steady_clock::now() + std::chrono::seconds( timeoutSeg );
         while ( std::chrono::steady_clock::now() < limite ) {
            char buffer[256];
            std::string ipOrigen;
            int puertoOrigen = 0;
            size_t n = udp.RecvFrom( buffer, sizeof( buffer ) - 1, &ipOrigen, &puertoOrigen );
            if ( 0 == n ) {
               break;   // timeout: siguiente intento
            }
            buffer[n] = '\0';
            int puertoTCP = PuertoDeRespuesta( buffer );
            if ( puertoTCP > 0 ) {
               resultado.ip = ipOrigen;
               resultado.puerto = puertoTCP;
               return true;
            }
         }
      }
   } catch ( const std::exception & e ) {
      std::cerr << "Error en el descubrimiento: " << e.what() << "\n";
   }
   return false;
}