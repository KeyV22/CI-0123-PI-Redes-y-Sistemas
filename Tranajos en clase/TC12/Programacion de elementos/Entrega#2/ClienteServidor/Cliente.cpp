#include <cstring>
#include <sstream>
#include <stdexcept>
#include "Cliente.hpp"

std::string Cliente::Get( const char * host, const char * servicio, const std::string & path ) {
   Socket s( 's', false );          // estamos usando ipv4, hay que cambiarlo a true para ipv6
   s.Connect( host, servicio );
   std::string request = "GET " + path + " HTTP/1.1\r\nhost: " + host + "\r\n\r\n";
   s.Write( request.c_str() );
   std::string completa = leerRespuestaCompleta( s );
   size_t finHeaders = completa.find("\r\n\r\n");
   std::string headers = (finHeaders == std::string::npos) ? completa : completa.substr(0, finHeaders);
   int codigo = extraerCodigoEstado( headers );
   if ( codigo < 200 || codigo >= 300 ) {
      throw std::runtime_error(
         "Cliente::Get - el servidor respondio codigo " + std::to_string(codigo)
      );
   }
   return extraerBody( completa );
}

// Get, pero manda el body como application/x-www-form-urlencoded
std::string Cliente::Post( const char * host, const char * servicio, const std::string & path, const std::string & cuerpo ) {
   Socket s( 's', false );
   s.Connect( host, servicio );

   std::ostringstream peticion;
   peticion << "POST " << path << " HTTP/1.1\r\n"
            << "host: " << host << "\r\n"
            << "Content-Type: application/x-www-form-urlencoded\r\n"
            << "Content-Length: " << cuerpo.size() << "\r\n"
            << "\r\n"
            << cuerpo;
   std::string request = peticion.str();
   s.Write( request.data(), request.size() );

   std::string completa = leerRespuestaCompleta( s );
   size_t finHeaders = completa.find("\r\n\r\n");
   std::string headers = (finHeaders == std::string::npos) ? completa : completa.substr(0, finHeaders);
   int codigo = extraerCodigoEstado( headers );
   if ( codigo < 200 || codigo >= 300 ) {
      throw std::runtime_error(
         "Cliente::Post - el servidor respondio codigo " + std::to_string(codigo)
      );
   }
   return extraerBody( completa );
}

// Lee del socket el ciclo hasta que el servidor cierra la conexión guardando todo en un string
std::string Cliente::leerRespuestaCompleta( Socket & s ) {
   std::string respuesta;
   char buf[512];
   int leido;
   int contentLength = -1;
   size_t inicioBody = std::string::npos;

   do {
      leido = s.Read( buf, sizeof(buf) );
      if ( leido > 0 ) {
         respuesta.append( buf, leido );

         // apenas tengamos los headers completos, calculamos cuánto body falta
         if ( inicioBody == std::string::npos ) {
            size_t fin = respuesta.find("\r\n\r\n");
            if ( fin != std::string::npos ) {
               inicioBody = fin + 4;
               contentLength = extraerContentLength( respuesta.substr(0, fin) );
            }
         }

         // si ya sabemos el Content-Length y ya tenemos suficiente body cortamos
         if ( inicioBody != std::string::npos && contentLength >= 0 ) {
            size_t bodyRecibido = respuesta.size() - inicioBody;
            if ( bodyRecibido >= (size_t)contentLength ) {
               break;
            }
         }
      }
   } while ( leido > 0 );

   return respuesta;
}

int Cliente::extraerContentLength( const std::string & headers ) {
   size_t pos = headers.find("Content-Length:");
   if ( pos == std::string::npos ) return -1;
   pos += strlen("Content-Length:");
   size_t fin = headers.find("\r\n", pos);
   std::string valor = headers.substr(pos, fin - pos);

   return std::stoi(valor);
}

std::string Cliente::extraerBody( const std::string & respuestaCompleta ) {
   size_t pos = respuestaCompleta.find("\r\n\r\n");
   if ( pos == std::string::npos ) return "";
   return respuestaCompleta.substr(pos + 4);
}

int Cliente::extraerCodigoEstado( const std::string & headers ) {
   size_t finPrimeraLinea = headers.find("\r\n");
   std::string primeraLinea = headers.substr(0, finPrimeraLinea);
   size_t primerEspacio = primeraLinea.find(' ');
   if ( primerEspacio == std::string::npos ) {
      throw std::runtime_error("Cliente::extraerCodigoEstado - linea de estado invalida");
   }
   size_t segundoEspacio = primeraLinea.find(' ', primerEspacio + 1);
   std::string codigoStr = primeraLinea.substr(primerEspacio + 1, segundoEspacio - primerEspacio - 1);
   return std::stoi(codigoStr);
}