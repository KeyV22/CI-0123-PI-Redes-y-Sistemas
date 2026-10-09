#include <iostream>
#include "Manejadorconexion.hpp"
#include "Httprequest.hpp"
#include "Httpresponse.hpp"
#include "Router.hpp"
#include <sstream>
#include "Bitacora.hpp"
#include "Parserproforma.hpp"
#include "Servidor.hpp"

std::string ManejadorConexion::LeerRequestCompleto( Socket & cliente ) {
   std::string acumulado;
   char buffer[ TAMANO_BUFFER ];

   while ( acumulado.size() < LIMITE_REQUEST ) {
      // Read() lanza runtime_error si read() devuelve -1 
      // esa excepcion se deja propagar hasta el try/catch de Atender().
      // Si el cliente cierra la conexion limpiamente, read() (y por lo
      // tanto Read()) devuelve 0 -- eso NO lanza excepcion.
      size_t leidos = cliente.Read( buffer, TAMANO_BUFFER );
      if ( leidos == 0 ) break;   // conexion cerrada por el cliente
      acumulado.append( buffer, leidos );

      // Ya tenemos los headers completos: revisamos si con eso alcanza
      // (GET sin body) o si falta leer el body segun Content-Length.
      size_t finHeaders = acumulado.find( "\r\n\r\n" );
      if ( finHeaders == std::string::npos ) continue;   // headers incompletos, seguir leyendo

      HttpRequest parcial = HttpRequest::Parsear( acumulado );
      if ( !parcial.valido ) break;   // se dejara que Router reporte el error 400

      auto it = parcial.headers.find( "content-length" );
      if ( it != parcial.headers.end() ) {
         try {
            size_t largo = (size_t)std::stoul( it->second );
            if ( parcial.cuerpo.size() >= largo ) break;   // ya llego todo el body
         } catch ( const std::exception & ) {
            break;   // Content-Length invalido: se corta aqui y el 400 lo reporta el Router
         }
      } else {
         break;   // sin Content-Length: se asume que no hay body (GET)
      }
   }
   return acumulado;
}

void ManejadorConexion::EscribirCompleto( Socket & cliente, const std::string & datos ) {
   size_t enviados = 0;
   while ( enviados < datos.size() ) {
      size_t escritos = cliente.Write( (const void *)( datos.data() + enviados ),datos.size() - enviados );
      if ( escritos == 0 ) break;   // no deberia pasar sin excepcion, pero por seguridad
      enviados += escritos;
   }
}

static std::string DescribirSolicitud( const HttpRequest & req ) {
   std::ostringstream out;
   out << req.metodo << " " << req.ruta;

   auto it = req.query.find( "category" );
   if ( it != req.query.end() ) {
      out << " | categoria: '" << it->second << "'";
   } else if ( req.metodo == "GET" && req.ruta == "/TicAmazon/list.php" ) {
      out << " | listar categorias";
   }

   if ( req.metodo == "POST" && req.ruta == "/TicAmazon/proforma.php" ) {
      std::vector<SolicitudItem> items = ParserProforma::Parsear( req.cuerpo );
      out << " | " << items.size() << " item(s): ";
      for ( size_t i = 0; i < items.size(); i++ ) {
         if ( i > 0 ) out << ", ";
         out << items[i].descripcion << " x" << items[i].cantidad
             << " (" << items[i].categoria << ")";
      }
   }
   return out.str();
}

void ManejadorConexion::Atender( Socket cliente, ServicioProductos & servicio ) {
   try {
      cliente.SetReadTimeout( TIMEOUT_LECTURA_SEGUNDOS );

      std::string crudo = LeerRequestCompleto( cliente );
      if ( crudo.empty() ) {
         return;   // el destructor de 'cliente' cierra el socket
      }

      HttpRequest req = HttpRequest::Parsear( crudo );
      RespuestaServicio resp = Router::Despachar( req, servicio );
      std::string respuestaCruda = HttpResponse::Construir( resp );

      EscribirCompleto( cliente, respuestaCruda );
      if ( req.valido ) {
         Bitacora::Registrar( DescribirSolicitud( req ) + " -> " + std::to_string( resp.codigo )+ " " + resp.frase );
      } else {
         Bitacora::Registrar( "Solicitud invalida (" + req.error + ") -> "+ std::to_string( resp.codigo ) );
      }
      if ( req.valido && req.metodo == "POST" && req.ruta == "/TicAmazon/salir" ) {
         Bitacora::Registrar( "Cierre del servidor solicitado desde el navegador" );
         Servidor::SolicitarApagado();
      }
   } catch ( const std::exception & e ) {
      Bitacora::Registrar( std::string( "Error atendiendo conexion: " ) + e.what() );
   }
}