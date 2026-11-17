#include <map>
#include <regex>
#include "Parserproforma.hpp"
#include "Httprequest.hpp"

std::vector<SolicitudItem> ParserProforma::Parsear( const std::string & cuerpo ) {
   std::map<std::string, std::string> campos = HttpRequest::ParsearFormUrlEncoded( cuerpo );

   // Agrupa por indice N a partir de las claves "itemN_categoria",
   // "itemN_producto", "itemN_cantidad".
   std::map<int, SolicitudItem> porIndice;
   std::regex patron( "^item(\\d+)_(categoria|producto|cantidad)$" );

   for ( const auto & par : campos ) {
      std::smatch m;
      if ( !std::regex_match( par.first, m, patron ) ) continue;   // clave que no es del carrito: se ignora

      int indice = std::stoi( m[1].str() );
      std::string campo = m[2].str();
      SolicitudItem & item = porIndice[ indice ];   // crea si no existe

      if ( campo == "categoria" ) {
         item.categoria = par.second;
      } else if ( campo == "producto" ) {
         item.descripcion = par.second;
      } else if ( campo == "cantidad" ) {
         try {
            item.cantidad = std::stoi( par.second );
         } catch ( const std::exception & ) {
            item.cantidad = -1;   // Inventario::GenerarProforma reporta "cantidad invalida"
         }
      }
   }

   std::vector<SolicitudItem> resultado;
   for ( const auto & par : porIndice ) {
      resultado.push_back( par.second );
   }
   return resultado;
}