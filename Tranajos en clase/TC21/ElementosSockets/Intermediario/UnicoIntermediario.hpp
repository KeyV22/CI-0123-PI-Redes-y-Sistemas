#ifndef UnicoIntermediario_hpp
#define UnicoIntermediario_hpp

#include <chrono>
#include <exception>
#include <string>
#include <vector>
#include "Socket.hpp"

// Regla: solo puede haber UN intermediario por isla.
// Antes de arrancar se pregunta por broadcast (mismo mensaje que usan los servidores de productos).
// Si alguien contesta, ya hay otro intermediario en la isla. Devuelve su IP en 'quien'.
inline bool HayOtroIntermediario( const std::vector<std::string> & broadcasts, std::string & quien,int puertoUdp = 7071, int segundos = 3 ) {
   try {
      Socket udp( 'd', false );
      udp.HabilitarBroadcast();
      udp.SetReadTimeout( 1 );
      for ( const std::string & b : broadcasts ) {
         try { udp.SendTo( "BUSCAR_INTERMEDIARIO", b, puertoUdp ); } catch ( const std::exception & ) {}
      }
      auto limite = std::chrono::steady_clock::now() + std::chrono::seconds( segundos );
      char buf[256];
      while ( std::chrono::steady_clock::now() < limite ) {
         std::string ip;
         int puerto = 0;
         long n = udp.RecvFrom( buf, sizeof( buf ) - 1, ip, puerto );
         if ( n <= 0 ) continue;
         std::string msg( buf, n );
         if ( msg.compare( 0, 14, "INTERMEDIARIO|" ) == 0 ) {
            quien = ip;
            return true;
         }
      }
   } catch ( const std::exception & ) {
   }
   return false;
}

#endif