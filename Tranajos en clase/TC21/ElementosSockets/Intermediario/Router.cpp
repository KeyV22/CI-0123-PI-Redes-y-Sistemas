#include <sstream>
#include "Router.hpp"
#include "Parserproforma.hpp"

const std::string Router::RUTA_LISTADO = "/TicAmazon/list.php";
const std::string Router::RUTA_PROFORMA = "/TicAmazon/proforma.php";
const std::string Router::RUTA_SERVIDORES = "/TicAmazon/servidores";
const std::string Router::RUTA_RUTAS = "/TicAmazon/rutas";
const std::string Router::RUTA_REGISTRAR = "/TicAmazon/registrar";
const std::string Router::RUTA_BITACORA = "/TicAmazon/bitacora";

static std::vector<std::string> Separar( const std::string & texto, char sep ) {
   std::vector<std::string> r;
   std::stringstream ss( texto );
   std::string x;
   while ( std::getline( ss, x, sep ) ) {
      size_t a = x.find_first_not_of( " \t" );
      if ( a == std::string::npos ) continue;
      size_t b = x.find_last_not_of( " \t" );
      r.push_back( x.substr( a, b - a + 1 ) );
   }
   return r;
}

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
      std::vector<SolicitudItem> items = ParserProforma::Parsear( req.cuerpo );
      return servicio.GenerarProforma( items );
   }

   if ( req.metodo == "GET" && req.ruta == RUTA_SERVIDORES ) {
      return servicio.ListarServidores();
   }

   if ( req.metodo == "GET" && req.ruta == RUTA_RUTAS ) {
      return servicio.ListarRutas();
   }

   if ( req.metodo == "GET" && req.ruta == RUTA_BITACORA ) {
      auto it = req.query.find( "tipo" );
      return servicio.ListarBitacora( it == req.query.end() ? "" : it->second );
   }

   if ( req.metodo == "POST" && req.ruta == RUTA_REGISTRAR ) {
      std::map<std::string, std::string> campos = HttpRequest::ParsearFormUrlEncoded( req.cuerpo );
      std::string host = campos["host"];
      int puerto = 0;
      try { puerto = std::stoi( campos["puerto"] ); } catch ( const std::exception & ) { puerto = 0; }
      if ( host.empty() || puerto <= 0 || puerto > 65535 ) {
         return servicio.Error( 400, "Registro invalido: se requieren 'host' y 'puerto'" );
      }
      return servicio.RegistrarServidor( campos["nombre"], host, puerto, Separar( campos["categorias"], ';' ) );
   }

   if ( req.ruta == RUTA_LISTADO || req.ruta == RUTA_PROFORMA || req.ruta == RUTA_SERVIDORES|| req.ruta == RUTA_RUTAS || req.ruta == RUTA_REGISTRAR || req.ruta == RUTA_BITACORA) {
      return servicio.Error( 405, "Metodo '" + req.metodo + "' no soportado en " + req.ruta );
   }

   return servicio.Error( 404, "Ruta no encontrada: " + req.ruta );
}