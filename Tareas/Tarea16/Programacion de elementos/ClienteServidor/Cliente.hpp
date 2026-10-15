#ifndef CLIENTE_HPP
#define CLIENTE_HPP

#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>
#include "Socket.hpp"

inline std::string UrlEncode( const std::string & texto ) {
   std::ostringstream encoded;
   for ( unsigned char c : texto ) {
      if ( isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' ) {
         encoded << c;
      } else {
         encoded << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c;
      }
   }
   return encoded.str();
}

class Cliente {
public:
    std::string Get( const char * host, const char * servicio, const std::string & path );

    // Igual que Get pero POST con Content-Type application/x-www-form-urlencoded.
    // 'cuerpo' ya debe venir armado y codificado (ver ArmarCuerpoProforma en main.cpp).
    std::string Post( const char * host, const char * servicio, const std::string & path, const std::string & cuerpo );

private:
    std::string leerRespuestaCompleta( Socket & s );
    std::string extraerBody( const std::string & respuestaCompleta );
    int extraerContentLength( const std::string & headers );
    int extraerCodigoEstado( const std::string & headers );
};

#endif