#include <cctype>
#include <cstdlib>
#include <sstream>
#include "Httprequest.hpp"
#include "Texto.hpp"

// Decodifica %XX y '+' como espacio, igual que hace cualquier servidor HTTP
// con una URL o con un body application/x-www-form-urlencoded.
std::string HttpRequest::DecodificarUrl( const std::string & texto ) {
   std::string resultado;
   for ( size_t i = 0; i < texto.size(); ++i ) {
      char c = texto[i];
      if ( c == '+' ) {
         resultado += ' ';
      } else if ( c == '%' && i + 2 < texto.size()
                  && std::isxdigit( (unsigned char)texto[i + 1] )
                  && std::isxdigit( (unsigned char)texto[i + 2] ) ) {
         std::string hex = texto.substr( i + 1, 2 );
         resultado += (char)std::strtol( hex.c_str(), nullptr, 16 );
         i += 2;
      } else {
         resultado += c;
      }
   }
   return resultado;
}

// Parsea "a=1&b=hola%20mundo" (usado tanto para el query string de la URL
// como para el body de la proforma).
std::map<std::string, std::string> HttpRequest::ParsearFormUrlEncoded( const std::string & texto ) {
   std::map<std::string, std::string> resultado;
   std::stringstream ss( texto );
   std::string par;
   while ( std::getline( ss, par, '&' ) ) {
      if ( par.empty() ) continue;
      size_t igual = par.find( '=' );
      std::string clave = ( igual == std::string::npos ) ? par : par.substr( 0, igual );
      std::string valor = ( igual == std::string::npos ) ? "" : par.substr( igual + 1 );
      resultado[ DecodificarUrl( clave ) ] = DecodificarUrl( valor );
   }
   return resultado;
}

std::string HttpRequest::ObtenerHeader( const std::map<std::string, std::string> & headers,const std::string & nombre ) {
   auto it = headers.find( Normalizar( nombre ) );
   return ( it == headers.end() ) ? "" : it->second;
}

HttpRequest HttpRequest::Parsear( const std::string & crudo ) {
   HttpRequest req;

   // Separa la linea de metodo de los headers usando el primer '\n'
   size_t finLinea = crudo.find( '\n' );
   if ( finLinea == std::string::npos ) {
      req.valido = false;
      req.error = "Request vacio o incompleto";
      return req;
   }

   std::string lineaMetodo = crudo.substr( 0, finLinea );
   if ( !lineaMetodo.empty() && lineaMetodo.back() == '\r' ) lineaMetodo.pop_back();

   std::istringstream lm( lineaMetodo );
   std::string urlCompleta, version;
   if ( !( lm >> req.metodo >> urlCompleta >> version ) ) {
      req.valido = false;
      req.error = "Linea de metodo mal formada: '" + lineaMetodo + "'";
      return req;
   }

   // Separa ruta de query string: /TicAmazon/list.php?category=Electronica
   size_t signo = urlCompleta.find( '?' );
   req.ruta = ( signo == std::string::npos ) ? urlCompleta : urlCompleta.substr( 0, signo );
   if ( signo != std::string::npos ) {
      req.query = ParsearFormUrlEncoded( urlCompleta.substr( signo + 1 ) );
   }

   // Headers: una linea "Clave: Valor" por linea, hasta encontrar una vacia
   size_t pos = finLinea + 1;
   size_t finHeaders = crudo.find( "\r\n\r\n", pos );
   bool separadorCorto = false;
   if ( finHeaders == std::string::npos ) {
      finHeaders = crudo.find( "\n\n", pos );
      separadorCorto = true;
      if ( finHeaders == std::string::npos ) finHeaders = crudo.size();
   }

   std::string bloqueHeaders = crudo.substr( pos, finHeaders - pos );
   std::istringstream hs( bloqueHeaders );
   std::string lineaHeader;
   while ( std::getline( hs, lineaHeader ) ) {
      if ( !lineaHeader.empty() && lineaHeader.back() == '\r' ) lineaHeader.pop_back();
      if ( lineaHeader.empty() ) continue;
      size_t dosPuntos = lineaHeader.find( ':' );
      if ( dosPuntos == std::string::npos ) continue;   // header mal formado: se ignora
      std::string clave = Normalizar( lineaHeader.substr( 0, dosPuntos ) );
      std::string valor = lineaHeader.substr( dosPuntos + 1 );
      // quita espacios al inicio del valor
      size_t inicioValor = valor.find_first_not_of( " \t" );
      valor = ( inicioValor == std::string::npos ) ? "" : valor.substr( inicioValor );
      req.headers[ clave ] = valor;
   }

   // Body: si hay Content-Length, se toma esa cantidad exacta de bytes
   // despues del separador de headers.
   size_t inicioBody = finHeaders + ( separadorCorto ? 2 : 4 );
   std::string clHeader = ObtenerHeader( req.headers, "content-length" );
   if ( !clHeader.empty() && inicioBody <= crudo.size() ) {
      try {
         size_t largo = (size_t)std::stoul( clHeader );
         size_t disponible = crudo.size() - inicioBody;
         req.cuerpo = crudo.substr( inicioBody, std::min( largo, disponible ) );
      } catch ( const std::exception & ) {
         req.valido = false;
         req.error = "Content-Length invalido: '" + clHeader + "'";
         return req;
      }
   } else if ( inicioBody < crudo.size() ) {
      req.cuerpo = crudo.substr( inicioBody );
   }

   req.valido = true;
   return req;
}