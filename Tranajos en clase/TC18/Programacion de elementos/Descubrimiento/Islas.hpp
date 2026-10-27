#ifndef Islas_hpp
#define Islas_hpp

#include <string>
#include <vector>

// Direcciones de broadcast de las islas (cada isla es una /28 de 172.16.123.0/24).
//   Isla 1: .31   Isla 2: .47   Isla 3: .63   Isla 4: .79   Isla 5: .95   Isla 6: .111
inline const std::vector<std::string> & BroadcastIslas() {
   static const std::vector<std::string> islas = {
      "172.16.123.31", "172.16.123.47", "172.16.123.63",
      "172.16.123.79", "172.16.123.95", "172.16.123.111"
   };
   return islas;
}

// Convierte el argumento de la linea de comandos en las direcciones a las que
// se manda la solicitud:
//   "isla3"  -> broadcast de la isla 3
//   "todas"  -> broadcast de las 6 islas (las ajenas requieren 'ip directed-broadcast' en el L3)
//   otro     -> se toma tal cual (ej. "172.16.123.63" o "127.255.255.255")
inline std::vector<std::string> ResolverBroadcast( const std::string & argumento ) {
   if ( argumento == "todas" ) {
      return BroadcastIslas();
   }
   if ( argumento.size() == 5 && argumento.compare( 0, 4, "isla" ) == 0 &&
        argumento[4] >= '1' && argumento[4] <= '6' ) {
      return { BroadcastIslas()[argumento[4] - '1'] };
   }
   return { argumento };
}

#endif