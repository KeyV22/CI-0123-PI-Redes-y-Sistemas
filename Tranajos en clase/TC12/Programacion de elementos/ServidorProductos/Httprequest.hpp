#ifndef HttpRequest_hpp
#define HttpRequest_hpp

#include <string>
#include <map>

// Representa un request HTTP ya parseado, listo para que el Router decida
// que hacer con el. No conoce nada de sockets: se construye a partir de
// un buffer de texto crudo
class HttpRequest {
   public:
      bool valido = false;          // false si el request esta mal formado
      std::string error;            // detalle del error si valido == false

      std::string metodo;           // "GET", "POST", ...
      std::string ruta;             // sin query string, ej: /TicAmazon/list.php
      std::map<std::string, std::string> query;    // parametros de la URL (?category=Electronica)
      std::map<std::string, std::string> headers;  // nombres de header en minuscula
      std::string cuerpo;           // body del request (para POST)

      // Parsea un request HTTP completo (linea de metodo + headers + body)
      // a partir del texto crudo recibido del socket.
      static HttpRequest Parsear( const std::string & crudo );

      // Utilidades de codificacion, reutilizadas tambien por el parser
      // de la proforma, asi que quedan publicas y estaticas.
      static std::string DecodificarUrl( const std::string & texto );
      static std::map<std::string, std::string> ParsearFormUrlEncoded( const std::string & texto );

   private:
      static std::string ObtenerHeader( const std::map<std::string, std::string> & headers,const std::string & nombre );
};

#endif