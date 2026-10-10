#include "Router.hpp"
#include "Parserproforma.hpp"

const std::string Router::RUTA_LISTADO = "/TicAmazon/list.php";
const std::string Router::RUTA_PROFORMA = "/TicAmazon/proforma.php";

RespuestaServicio Router::Despachar( const HttpRequest & req, ServicioProductos & servicio ) {
   if ( !req.valido ) {
      return servicio.Error( 400, req.error );
   }

   if ( req.metodo == "GET" && req.ruta == RUTA_LISTADO ) {
      auto it = req.query.find( "category" );
      if ( it == req.query.end() ) {
         return servicio.ListarCategorias();
      }
      return servicio.ListarProductos( it->second );
   }

   if ( req.metodo == "POST" && req.ruta == RUTA_PROFORMA ) {
      // El body llega como application/x-www-form-urlencoded; ver
      // ParserProforma (commit 5) para el formato exacto esperado.
      std::vector<SolicitudItem> items = ParserProforma::Parsear( req.cuerpo );
      return servicio.GenerarProforma( items );
   }

   // Ruta conocida pero metodo incorrecto (ej. POST a list.php)
   if ( req.ruta == RUTA_LISTADO || req.ruta == RUTA_PROFORMA ) {
      return servicio.Error( 405, "Metodo '" + req.metodo + "' no soportado en " + req.ruta );
   }

   return servicio.Error( 404, "Ruta no encontrada: " + req.ruta );
}