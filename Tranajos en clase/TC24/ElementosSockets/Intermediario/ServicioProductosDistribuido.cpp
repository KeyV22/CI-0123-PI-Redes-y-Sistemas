#include <chrono>
#include <cstdlib>
#include <cstring>
#include <map>
#include <thread>
#include "ServicioProductosDistribuido.hpp"
#include "HttpCliente.hpp"
#include "Bitacora.hpp"
#include "Texto.hpp"
#include "Mancomunado.hpp"

static std::string Desescapar( std::string t ) {
   const char * de[] = { "&lt;", "&gt;", "&quot;", "&amp;" };
   const char * a[]  = { "<", ">", "\"", "&" };
   for ( int i = 0; i < 4; i++ ) {
      std::string d = de[i];
      size_t p = 0;
      while ( ( p = t.find( d, p ) ) != std::string::npos ) {
         t.replace( p, d.size(), a[i] );
         p += std::strlen( a[i] );
      }
   }
   return t;
}

// Devuelve las filas <TR> que tienen celdas <TD> (la fila de <TH> se ignora)
static std::vector<std::vector<std::string>> ParsearFilas( const std::string & html ) {
   std::vector<std::vector<std::string>> filas;
   size_t pos = 0;
   while ( ( pos = html.find( "<TR>", pos ) ) != std::string::npos ) {
      size_t fin = html.find( "</TR>", pos );
      if ( fin == std::string::npos ) break;
      std::string fila = html.substr( pos + 4, fin - pos - 4 );
      std::vector<std::string> celdas;
      size_t p = 0;
      while ( ( p = fila.find( "<TD>", p ) ) != std::string::npos ) {
         size_t c = fila.find( "</TD>", p );
         if ( c == std::string::npos ) break;
         celdas.push_back( Desescapar( fila.substr( p + 4, c - p - 4 ) ) );
         p = c + 5;
      }
      if ( !celdas.empty() ) filas.push_back( celdas );
      pos = fin + 5;
   }
   return filas;
}

static std::vector<std::string> ParsearCategorias( const std::string & html ) {
   std::vector<std::string> cats;
   size_t pos = 0;
   while ( ( pos = html.find( "<A href=", pos ) ) != std::string::npos ) {
      size_t ini = html.find( '>', pos );
      if ( ini == std::string::npos ) break;
      size_t fin = html.find( "</A>", ini );
      if ( fin == std::string::npos ) break;
      cats.push_back( Desescapar( html.substr( ini + 1, fin - ini - 1 ) ) );
      pos = fin + 4;
   }
   return cats;
}

// Items <LI> (lista de errores de una proforma rechazada)
static std::vector<std::string> ParsearItemsLista( const std::string & html ) {
   std::vector<std::string> r;
   size_t pos = 0;
   while ( ( pos = html.find( "<LI>", pos ) ) != std::string::npos ) {
      size_t fin = html.find( "</LI>", pos );
      if ( fin == std::string::npos ) break;
      r.push_back( Desescapar( html.substr( pos + 4, fin - pos - 4 ) ) );
      pos = fin + 5;
   }
   return r;
}

//  monitoreo y registro 

void ServicioProductosDistribuido::AgregarSemilla( const std::string & host, int puerto ) {
   registro.AgregarSemilla( host, puerto );
}

void ServicioProductosDistribuido::DescubrirServidor( const std::string & host, int puerto, const std::string & nombre ) {
   for ( const InfoServidor & s : registro.Todos() ) {
      if ( s.host == host && s.puerto == puerto && s.activo ) return;
   }
   registro.AgregarSemilla( host, puerto );

   RespuestaHttp r = HttpCliente::Enviar( host, puerto, "GET", "/TicAmazon/list.php", "", 3 );
   if ( r.ok && r.codigo == 200 ) {
            std::vector<std::string> cats = ParsearCategorias( r.cuerpo );
      std::string detalle;
      if ( HayConflicto( host, puerto, nombre, cats, detalle ) ) return;
      if ( registro.Registrar( nombre, host, puerto, cats ) ) {
         Bitacora::Registrar( "REGISTRO", "Servidor de productos descubierto por broadcast: " + nombre+ " (" + host + ":" + std::to_string( puerto ) + ")", nombre );
      }
   }
}

bool ServicioProductosDistribuido::HayConflicto( const std::string & host, int puerto, const std::string & nombre,const std::vector<std::string> & categorias, std::string & detalle ) const {
   detalle = registro.Conflicto( host, puerto, categorias, nombre );
   if ( detalle.empty() ) return false;
   std::string clave = host + ":" + std::to_string( puerto ) + "|" + detalle;
   bool nuevo;
   {
      std::lock_guard<std::mutex> lock( mtxConflictos );
      nuevo = conflictosAvisados.insert( clave ).second;
   }
   if ( nuevo ) {
      std::string quien = nombre.empty() ? host + ":" + std::to_string( puerto ) : nombre;
      Bitacora::Registrar( "RECHAZADO", "Servidor de productos rechazado (" + quien + "): " + detalle, quien );
   }
   return true;
}

bool ServicioProductosDistribuido::SondearServidor( const InfoServidor & s ) const {
   RespuestaHttp r = HttpCliente::Enviar( s.host, s.puerto, "GET", "/TicAmazon/list.php", "", 3 );
   if ( r.ok && r.codigo == 200 ) {
      std::vector<std::string> cats = ParsearCategorias( r.cuerpo );
      std::string detalle;
      if ( HayConflicto( s.host, s.puerto, s.nombre, cats, detalle ) ) return false;
      if ( registro.Registrar( "", s.host, s.puerto, cats ) ) {
         Bitacora::Registrar( "ACTIVO", "Servidor de productos activo: " + s.nombre, s.nombre );
      }
      return true;
   }
   if ( registro.MarcarInactivo( s.host, s.puerto ) ) {
      Bitacora::Registrar( "INACTIVO", "Servidor de productos inactivo: " + s.nombre, s.nombre );   }
   return false;
}

void ServicioProductosDistribuido::Sondear() const {
   for ( const InfoServidor & s : registro.Todos() ) {
      SondearServidor( s );
   }
}

void ServicioProductosDistribuido::IniciarMonitor( int segundos ) {
   std::thread( [this, segundos]() {
      while ( true ) {
         Sondear();
         std::this_thread::sleep_for( std::chrono::seconds( segundos ) );
      }
   } ).detach();
}

RespuestaServicio ServicioProductosDistribuido::RegistrarServidor( const std::string & nombre, const std::string & host,int puerto, const std::vector<std::string> & categorias ) const {
   std::string detalle;
   if ( HayConflicto( host, puerto, nombre, categorias, detalle ) ) {
      return Error( 409, "Registro rechazado: " + detalle + ". Los servidores de una misma isla deben tener categorias distintas" );
   }
   
   if ( registro.Registrar( nombre, host, puerto, categorias ) ) {
      Bitacora::Registrar( "REGISTRO", "Servidor de productos registrado: " + ( nombre.empty() ? host : nombre )+ " (" + host + ":" + std::to_string( puerto ) + ") con "+ std::to_string( categorias.size() ) + " categoria(s)",nombre.empty() ? host : nombre );   }
   return Construir( 200, PaginasHtml::PaginaMensaje( "TicAmazon - Registro", "Servidor registrado correctamente" ) );
}

RespuestaServicio ServicioProductosDistribuido::DesregistrarServidor( const std::string & host, int puerto ) const {
   if ( registro.MarcarInactivo( host, puerto ) ) {
      Bitacora::Registrar( "INACTIVO", "Servidor de productos se despidio: " + host + ":" + std::to_string( puerto ), host + ":" + std::to_string( puerto ) );
   }
   return Construir( 200, PaginasHtml::PaginaMensaje( "TicAmazon - Desregistro", "Servidor desregistrado" ) );
}

RespuestaServicio ServicioProductosDistribuido::ListarServidores() const {
   return Construir( 200, PaginasHtml::PaginaServidores( registro.Todos() ) );
}

RespuestaServicio ServicioProductosDistribuido::ListarRutas() const {
   auto rutas = registro.Rutas();
   // Rutas hacia otras islas: categoria -> "INT_xx (ip:puerto)"
   if ( islasRemotas != nullptr ) {
      for ( const InfoIsla & isla : islasRemotas->Todas() ) {
         if ( !isla.activa ) continue;
         for ( const std::string & c : isla.categorias ) {
            size_t idx = rutas.size();
            for ( size_t i = 0; i < rutas.size(); i++ ) {
               if ( Mancomunado::NombreProtocolo( rutas[i].first ) == Mancomunado::NombreProtocolo( c ) ) { idx = i; break; }
            }
            if ( idx == rutas.size() ) rutas.push_back( { c, {} } );
            rutas[idx].second.push_back( isla.id + " (" + isla.host + ":" + std::to_string( isla.puerto ) + ")" );
         }
      }
   }
   return Construir( 200, PaginasHtml::PaginaRutas( rutas ) );
}

RespuestaServicio ServicioProductosDistribuido::ListarBitacora( const std::string & filtroTipo ) const {
   return Construir( 200, PaginasHtml::PaginaBitacora( Bitacora::Ultimos( filtroTipo, 100 ),Bitacora::Resumen(), filtroTipo ) );
}

//  consultas 

bool ServicioProductosDistribuido::ConsultarProductos( const InfoServidor & s, const std::string & categoria,std::vector<FilaProducto> & filas ) const {
    RespuestaHttp r = HttpCliente::Enviar( s.host, s.puerto, "GET","/TicAmazon/list.php?category=" + PaginasHtml::CodificarUrl( categoria ) );
   if ( !r.ok ) {
      if ( registro.MarcarInactivo( s.host, s.puerto ) ) {
        Bitacora::Registrar( "INACTIVO", "Servidor de productos inactivo: " + s.nombre + " (" + r.error + ")", s.nombre );      }
      return false;
   }
   if ( r.codigo != 200 ) return false;

   for ( const std::vector<std::string> & c : ParsearFilas( r.cuerpo ) ) {
      if ( c.size() < 6 ) continue;
      FilaProducto f;
      f.intermediario = s.nombre;   
      f.bodega = c[1];
      f.categoria = c[2];
      f.descripcion = c[3];
      f.cantidad = c[4];
      f.precio = c[5];
      filas.push_back( f );
   }
   return true;
}

void ServicioProductosDistribuido::FijarMancomunado( Mancomunado * m, RegistroIslas * islas ) {
   mancomunado = m;
   islasRemotas = islas;
}

std::vector<std::string> ServicioProductosDistribuido::CategoriasLocales() const {
   std::vector<std::string> cats;
   for ( const InfoServidor & s : registro.Activos() ) {
      for ( const std::string & c : s.categorias ) {
         bool existe = false;
         for ( const std::string & x : cats ) {
            if ( IgualesSinMayusculas( x, c ) ) { existe = true; break; }
         }
         if ( !existe ) cats.push_back( c );
      }
   }
   return cats;
}

void ServicioProductosDistribuido::ProductosLocales( const std::string & categoria, std::vector<FilaProducto> & filas ) const {
   for ( const InfoServidor & s : registro.ServidoresDeCategoria( categoria ) ) {
      ConsultarProductos( s, categoria, filas );
   }
}

RespuestaServicio ServicioProductosDistribuido::ListarCategorias() const {
   std::vector<std::string> cats = CategoriasLocales();
   // Categorias de otras islas (tabla de rutas hacia las islas), sin repetir las que ya hay aqui
   if ( islasRemotas != nullptr ) {
      for ( const InfoIsla & isla : islasRemotas->Todas() ) {
         if ( !isla.activa ) continue;
         for ( const std::string & c : isla.categorias ) {
            bool existe = false;
            for ( const std::string & x : cats ) {
               if ( Mancomunado::NombreProtocolo( x ) == Mancomunado::NombreProtocolo( c ) ) { existe = true; break; }
            }
            if ( !existe ) cats.push_back( c );
         }
      }
   }
   return Construir( 200, PaginasHtml::PaginaCategorias( cats ) );
}

RespuestaServicio ServicioProductosDistribuido::ListarProductos( const std::string & categoria ) const {
   if ( categoria.empty() ) {
      return Error( 400, "Falta el parametro 'category'" );
   }
   std::vector<FilaProducto> filas;
   ProductosLocales( categoria, filas );
   // Si otra isla tiene la categoria, se le pide con GET_PRODUCTS (20)
   if ( islasRemotas != nullptr && mancomunado != nullptr ) {
      std::string nombre = Mancomunado::NombreProtocolo( categoria );
      for ( const InfoIsla & isla : islasRemotas->IslasDeCategoria( nombre ) ) {
         if ( !mancomunado->ConsultarProductos( isla, nombre, filas ) ) {
            if ( islasRemotas->MarcarInactiva( isla.id ) ) {
               Bitacora::Registrar( "INACTIVO", "Intermediario inactivo (ruta inhabilitada): " + isla.id, isla.id );
            }
         }
      }
   }
   return Construir( 200, PaginasHtml::PaginaProductosDistribuidos( categoria, filas ) );
}

RespuestaServicio ServicioProductosDistribuido::GenerarProforma( const std::vector<SolicitudItem> & items ) const {
   if ( items.empty() ) {
      return Error( 400, "La solicitud de proforma no contiene productos" );
   }

   Proforma validacion;
   std::map<std::string, std::vector<FilaProducto>> cache;     // servidor|categoria -> filas
   std::map<std::string, std::vector<SolicitudItem>> porServidor;
   std::map<std::string, InfoServidor> infoServidor;
   std::map<std::string, int> pedido;                          // servidor|producto -> unidades ya asignadas

   for ( const SolicitudItem & it : items ) {
      if ( it.cantidad <= 0 ) {
         validacion.AgregarError( "Cantidad invalida para '" + it.descripcion + "'" );
         continue;
      }
      std::vector<InfoServidor> candidatos = registro.ServidoresDeCategoria( it.categoria );
      if ( candidatos.empty() ) {
         validacion.AgregarError( "La categoria '" + it.categoria + "' no existe en ningun servidor activo" );
         continue;
      }

      bool encontrado = false, asignado = false;
      for ( const InfoServidor & s : candidatos ) {
         std::string clave = s.host + ":" + std::to_string( s.puerto );
         std::string claveCache = clave + "|" + Normalizar( it.categoria );
         auto c = cache.find( claveCache );
         if ( c == cache.end() ) {
            std::vector<FilaProducto> filas;
            ConsultarProductos( s, it.categoria, filas );
            c = cache.emplace( claveCache, filas ).first;
         }
         for ( const FilaProducto & f : c->second ) {
            if ( !IgualesSinMayusculas( f.descripcion, it.descripcion ) ) continue;
            encontrado = true;
            int stock = std::atoi( f.cantidad.c_str() );
            std::string clavePed = clave + "|" + Normalizar( it.descripcion );
            if ( pedido[clavePed] + it.cantidad <= stock ) {
               pedido[clavePed] += it.cantidad;
               porServidor[clave].push_back( it );
               infoServidor[clave] = s;
               asignado = true;
            }
            break;  
         }
         if ( asignado ) break;
      }
      if ( !asignado ) {
         if ( encontrado ) {
            validacion.AgregarError( "Stock insuficiente para '" + it.descripcion + "' en los servidores activos" );
         } else {
            validacion.AgregarError( "El producto '" + it.descripcion + "' no existe en la categoria '" + it.categoria + "'" );
         }
      }
   }
   if ( !validacion.EsValida() ) {
      return Construir( 422, PaginasHtml::PaginaProforma( validacion ) );
   }
   Proforma resultado;
   for ( const auto & par : porServidor ) {
      const InfoServidor & s = infoServidor.at( par.first );
      std::string cuerpo;
      for ( size_t i = 0; i < par.second.size(); i++ ) {
         if ( i > 0 ) cuerpo += "&";
         std::string n = std::to_string( i );
         cuerpo += "item" + n + "_categoria=" + PaginasHtml::CodificarUrl( par.second[i].categoria )
                 + "&item" + n + "_producto=" + PaginasHtml::CodificarUrl( par.second[i].descripcion )
                 + "&item" + n + "_cantidad=" + std::to_string( par.second[i].cantidad );
      }

      RespuestaHttp r = HttpCliente::Enviar( s.host, s.puerto, "POST", "/TicAmazon/proforma.php", cuerpo );
      if ( !r.ok ) {
         registro.MarcarInactivo( s.host, s.puerto );
         Bitacora::Registrar( "Servidor de productos INACTIVO: " + s.nombre + " (" + r.error + ")" );
         resultado.AgregarError( "No se pudo contactar al servidor '" + s.nombre + "': sus productos NO se compraron" );
         continue;
      }
      if ( r.codigo != 200 ) {
         std::vector<std::string> errs = ParsearItemsLista( r.cuerpo );
         if ( errs.empty() ) errs.push_back( "El servidor respondio con codigo " + std::to_string( r.codigo ) );
         for ( const std::string & e : errs ) {
            resultado.AgregarError( "[" + s.nombre + "] " + e );
         }
         continue;
      }
      try {
         for ( const std::vector<std::string> & c : ParsearFilas( r.cuerpo ) ) {
            if ( c.size() < 6 ) continue;
            LineaProforma l;
            l.bodega = c[0];
            l.categoria = c[1];
            l.descripcion = c[2];
            l.cantidad = std::stoi( c[3] );
            l.precioUnitario = std::stod( c[4] );
            resultado.AgregarLinea( l );
         }
      } catch ( const std::exception & ) {
         resultado.AgregarError( "Respuesta de proforma invalida desde '" + s.nombre + "'" );
      }
   }
   if ( !resultado.EsValida() && !resultado.Lineas().empty() ) {
      resultado.AgregarError( "Atencion: estas lineas YA se confirmaron y descontaron en otros servidores:" );
      for ( const LineaProforma & l : resultado.Lineas() ) {
         resultado.AgregarError( l.descripcion + " x" + std::to_string( l.cantidad ) + " (" + l.bodega + ")" );
      }
   }
   int codigo = resultado.EsValida() ? 200 : 422;
   return Construir( codigo, PaginasHtml::PaginaProforma( resultado ) );
}