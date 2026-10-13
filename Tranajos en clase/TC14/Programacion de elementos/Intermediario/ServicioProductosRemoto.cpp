#include <cstdio>
#include <exception>
#include <sstream>
#include "ServicioProductosRemoto.hpp"
#include "Socket.hpp"

std::string ServicioProductosRemoto::CodificarUrl( const std::string & texto ) {
   std::string salida;
   char buf[4];
   for ( unsigned char c : texto ) {
      if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
           ( c >= '0' && c <= '9' ) || c == '-' || c == '_' || c == '.' || c == '~' ) {
         salida += (char)c;
      } else {
         std::snprintf( buf, sizeof( buf ), "%%%02X", c );
         salida += buf;
      }
   }
   return salida;
}

RespuestaServicio ServicioProductosRemoto::Reenviar( const std::string & metodo, const std::string & ruta,
                                                     const std::string & cuerpo ) const {
   try {
      Socket s( 's', false );
      s.SetReadTimeout( 10 );
      s.Connect( host.c_str(), puerto );

      std::ostringstream peticion;
      peticion << metodo << " " << ruta << " HTTP/1.1\r\n"
               << "Host: " << host << "\r\n"
               << "Connection: close\r\n";
      if ( metodo == "POST" ) {
         peticion << "Content-Type: application/x-www-form-urlencoded\r\n"
                  << "Content-Length: " << cuerpo.size() << "\r\n";
      }
      peticion << "\r\n" << cuerpo;
      std::string request = peticion.str();
      s.Write( request.data(), request.size() );

      // Lee hasta tener headers completos y luego Content-Length bytes de body
      std::string respuesta;
      char buf[4096];
      size_t finHeaders = std::string::npos;
      size_t largo = 0;
      while ( true ) {
         size_t leidos = s.Read( buf, sizeof( buf ) );
         if ( leidos == 0 ) break;
         respuesta.append( buf, leidos );
         if ( finHeaders == std::string::npos ) {
            finHeaders = respuesta.find( "\r\n\r\n" );
            if ( finHeaders != std::string::npos ) {
               std::string cab = respuesta.substr( 0, finHeaders );
               size_t pos = cab.find( "Content-Length:" );
               if ( pos != std::string::npos ) largo = std::stoul( cab.substr( pos + 15 ) );
            }
         }
         if ( finHeaders != std::string::npos && respuesta.size() >= finHeaders + 4 + largo ) break;
      }
      if ( finHeaders == std::string::npos ) {
         return Error( 500, "Respuesta invalida del servidor de productos" );
      }

      RespuestaServicio r;
      // Linea de estado: "HTTP/1.1 200 OK"
      size_t sp1 = respuesta.find( ' ' );
      size_t sp2 = respuesta.find( ' ', sp1 + 1 );
      r.codigo = std::stoi( respuesta.substr( sp1 + 1, sp2 - sp1 - 1 ) );
      r.frase = Frase( r.codigo );
      r.tipoContenido = "text/html; charset=utf-8";
      r.cuerpo = respuesta.substr( finHeaders + 4, largo );
      return r;
   } catch ( const std::exception & e ) {
      return Error( 500, std::string( "No se pudo contactar al servidor de productos: " ) + e.what() );
   }
}

RespuestaServicio ServicioProductosRemoto::ListarCategorias() const {
   return Reenviar( "GET", "/TicAmazon/list.php", "" );
}

RespuestaServicio ServicioProductosRemoto::ListarProductos( const std::string & categoria ) const {
   if ( categoria.empty() ) {
      return Error( 400, "Falta el parametro 'category'" );
   }
   return Reenviar( "GET", "/TicAmazon/list.php?category=" + CodificarUrl( categoria ), "" );
}

RespuestaServicio ServicioProductosRemoto::GenerarProforma( const std::vector<SolicitudItem> & items ) const {
   if ( items.empty() ) {
      return Error( 400, "La solicitud de proforma no contiene productos" );
   }
   // Se vuelve a armar el mismo form-urlencoded que ParserProforma sabe leer
   std::string cuerpo;
   for ( size_t i = 0; i < items.size(); i++ ) {
      if ( i > 0 ) cuerpo += "&";
      std::string n = std::to_string( i );
      cuerpo += "item" + n + "_categoria=" + CodificarUrl( items[i].categoria )
              + "&item" + n + "_producto=" + CodificarUrl( items[i].descripcion )
              + "&item" + n + "_cantidad=" + std::to_string( items[i].cantidad );
   }
   return Reenviar( "POST", "/TicAmazon/proforma.php", cuerpo );
}