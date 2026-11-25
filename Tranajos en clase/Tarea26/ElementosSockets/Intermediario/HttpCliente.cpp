#include <algorithm>
#include <cctype>
#include <exception>
#include <sstream>
#include "HttpCliente.hpp"
#include "SSLSocket.hpp"

RespuestaHttp HttpCliente::Enviar( const std::string & host, int puerto,const std::string & metodo, const std::string & ruta,const std::string & cuerpo, int timeoutSeg ) {
   RespuestaHttp r;
   try {
      SSLSocket s;
      s.SetReadTimeout( timeoutSeg );    
      s.Connect( host.c_str(), puerto );
      std::ostringstream peticion;
      peticion << metodo << " " << ruta << " HTTP/1.1\r\n"<< "Host: " << host << "\r\n"<< "Connection: close\r\n";
      if ( metodo == "POST" ) {
         peticion << "Content-Type: application/x-www-form-urlencoded\r\n"<< "Content-Length: " << cuerpo.size() << "\r\n";
      }
      peticion << "\r\n" << cuerpo;
      std::string request = peticion.str();
      s.Write( request.data(), request.size() );
      std::string resp;
      char buf[4096];
      size_t finHeaders = std::string::npos;
      size_t largo = 0;
      bool hayLargo = false;
      while ( true ) {
         size_t n = s.Read( buf, sizeof( buf ) );
         if ( n == 0 ) break;
         resp.append( buf, n );
         if ( finHeaders == std::string::npos ) {
            finHeaders = resp.find( "\r\n\r\n" );
            if ( finHeaders != std::string::npos ) {
               std::string cab = resp.substr( 0, finHeaders );
               std::transform( cab.begin(), cab.end(), cab.begin(),[]( unsigned char c ) { return (char)std::tolower( c ); } );
               size_t pos = cab.find( "content-length:" );
               if ( pos != std::string::npos ) {
                  largo = std::stoul( cab.substr( pos + 15 ) );
                  hayLargo = true;
               }
            }
         }
         if ( finHeaders != std::string::npos && hayLargo && resp.size() >= finHeaders + 4 + largo ) break;
      }
      if ( finHeaders == std::string::npos ) {
         r.error = "Respuesta invalida";
         return r;
      }
      size_t sp1 = resp.find( ' ' );
      size_t sp2 = resp.find( ' ', sp1 + 1 );
      r.codigo = std::stoi( resp.substr( sp1 + 1, sp2 - sp1 - 1 ) );
      r.cuerpo = hayLargo ? resp.substr( finHeaders + 4, largo ) : resp.substr( finHeaders + 4 );
      r.ok = true;
   } catch ( const std::exception & e ) {
      r.error = e.what();
   }
   return r;
}