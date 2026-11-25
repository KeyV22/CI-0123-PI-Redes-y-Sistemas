#ifndef Broadcasts_hpp
#define Broadcasts_hpp

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

// Broadcast de las interfaces IPv4 activas de esta maquina.
//  - Si hay una interfaz en la red PRIVADA (172.16.123.x) se usa SOLO esa: nunca se manda
//    broadcast por la red publica ni por otras interfaces.
//  - Si no la hay (pruebas en una PC cualquiera) se usan todas, mas el de loopback.
inline std::vector<std::string> DetectarBroadcasts() {
   std::vector<std::string> privadas, todas;
   struct ifaddrs * lista = nullptr;
   if ( getifaddrs( &lista ) == 0 ) {
      for ( struct ifaddrs * a = lista; a != nullptr; a = a->ifa_next ) {
         if ( !a->ifa_addr || a->ifa_addr->sa_family != AF_INET ) continue;
         if ( !( a->ifa_flags & IFF_UP ) || !( a->ifa_flags & IFF_BROADCAST ) ) continue;
         if ( !a->ifa_broadaddr ) continue;
         char propia[INET_ADDRSTRLEN], ip[INET_ADDRSTRLEN];
         inet_ntop( AF_INET, &( (struct sockaddr_in *)a->ifa_addr )->sin_addr, propia, sizeof( propia ) );
         inet_ntop( AF_INET, &( (struct sockaddr_in *)a->ifa_broadaddr )->sin_addr, ip, sizeof( ip ) );
         if ( std::string( propia ).compare( 0, 11, "172.16.123." ) == 0 ) {
            if ( std::find( privadas.begin(), privadas.end(), ip ) == privadas.end() ) privadas.push_back( ip );
         }
         if ( std::find( todas.begin(), todas.end(), ip ) == todas.end() ) todas.push_back( ip );
      }
      freeifaddrs( lista );
   }
   if ( !privadas.empty() ) return privadas;
   return todas;
}

// Sin argumento (o "auto") -> deteccion automatica.
// Con argumento -> lista de IPs separadas por coma.
inline std::vector<std::string> ResolverBroadcasts( const std::string & arg ) {
   if ( arg.empty() || arg == "auto" ) return DetectarBroadcasts();
   std::vector<std::string> r;
   std::stringstream ss( arg );
   std::string x;
   while ( std::getline( ss, x, ',' ) ) {
      if ( !x.empty() ) r.push_back( x );
   }
   return r;
}

#endif