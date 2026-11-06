#include "ServicioProductosLocal.hpp"
#include "PaginasHtml.hpp"

RespuestaServicio ServicioProductosLocal::ListarCategorias() const {
   return Construir( 200, PaginasHtml::PaginaCategorias( inventario.ListarCategorias() ) );
}

RespuestaServicio ServicioProductosLocal::ListarProductos( const std::string & categoria ) const {
   if ( categoria.empty() ) {
      return Error( 400, "Falta el parametro 'category'" );
   }
   // Categoria desconocida: 200 con tabla vacia (el cliente ya maneja ese caso)
   return Construir( 200, PaginasHtml::PaginaProductos( categoria, inventario.ListarProductos( categoria ) ) );
}

RespuestaServicio ServicioProductosLocal::GenerarProforma( const std::vector<SolicitudItem> & items ) const {
   if ( items.empty() ) {
      return Error( 400, "La solicitud de proforma no contiene productos" );
   }
   Proforma pf = inventario.GenerarProforma( items );
   int codigo = pf.EsValida() ? 200 : 422;
   return Construir( codigo, PaginasHtml::PaginaProforma( pf ) );
}