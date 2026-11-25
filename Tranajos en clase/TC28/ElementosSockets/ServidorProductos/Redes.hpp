#ifndef Redes_hpp
#define Redes_hpp

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <string>

// Las dos redes del intermediario
//   privada (islas):  172.16.123.x   -> descubrimiento y protocolo mancomunado
//   publica (clientes): 10.1.35.x     -> HTTP hacia los clientes
static const char * const PREFIJO_RED_PRIVADA = "172.16.123.";
static const char * const PREFIJO_RED_PUBLICA = "10.1.35.";

inline bool EmpiezaCon( const std::string & texto, const std::string & prefijo ) {
   return texto.compare( 0, prefijo.size(), prefijo ) == 0;
}

// IP de la primera interfaz IPv4 cuya direccion empieza con 'prefijo' ("" si no hay)
inline std::string IpConPrefijo( const std::string & prefijo ) {
   std::string resultado;
   struct ifaddrs * lista = nullptr;
   if ( getifaddrs( &lista ) != 0 ) return resultado;
   for ( struct ifaddrs * a = lista; a != nullptr && resultado.empty(); a = a->ifa_next ) {
      if ( !a->ifa_addr || a->ifa_addr->sa_family != AF_INET ) continue;
      char buf[INET_ADDRSTRLEN];
      inet_ntop( AF_INET, &( (struct sockaddr_in *)a->ifa_addr )->sin_addr, buf, sizeof( buf ) );
      if ( EmpiezaCon( buf, prefijo ) ) resultado = buf;
   }
   freeifaddrs( lista );
   return resultado;
}

#endif