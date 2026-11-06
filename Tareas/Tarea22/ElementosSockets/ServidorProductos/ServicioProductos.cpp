#include "ServicioProductos.hpp"
#include "PaginasHtml.hpp"

std::string ServicioProductos::Frase( int codigo ) {
   switch ( codigo ) {
      case 200: return "OK";
      case 400: return "Bad Request";
      case 404: return "Not Found";
      case 405: return "Method Not Allowed";
      case 422: return "Unprocessable Entity";
      case 500: return "Internal Server Error";
      default:  return "Unknown";
   }
}

RespuestaServicio ServicioProductos::Construir( int codigo, const std::string & cuerpo ) {
   RespuestaServicio r;
   r.codigo = codigo;
   r.frase = Frase( codigo );
   r.tipoContenido = "text/html; charset=utf-8";
   r.cuerpo = cuerpo;
   return r;
}

RespuestaServicio ServicioProductos::Error( int codigo, const std::string & detalle ) const {
   return Construir( codigo, PaginasHtml::PaginaError( codigo, detalle ) );
}