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

RespuestaServicio ServicioProductos::ListarCategorias() const {
   return Construir( 200, PaginasHtml::PaginaCategorias( inventario.ListarCategorias() ) );
}

RespuestaServicio ServicioProductos::ListarProductos( const std::string & categoria ) const {
   if ( categoria.empty() ) {
      return Error( 400, "Falta el parametro 'category'" );
   }
   // Categoria desconocida: 200 con tabla vacia (el cliente ya maneja ese caso)
   return Construir( 200, PaginasHtml::PaginaProductos( categoria, inventario.ListarProductos( categoria ) ) );
}

RespuestaServicio ServicioProductos::GenerarProforma( const std::vector<SolicitudItem> & items ) const {
   if ( items.empty() ) {
      return Error( 400, "La solicitud de proforma no contiene productos" );
   }
   Proforma pf = inventario.GenerarProforma( items );
   int codigo = pf.EsValida() ? 200 : 422;
   return Construir( codigo, PaginasHtml::PaginaProforma( pf ) );
}

RespuestaServicio ServicioProductos::Error( int codigo, const std::string & detalle ) const {
   return Construir( codigo, PaginasHtml::PaginaError( codigo, detalle ) );
}