#include <cctype>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include "PaginasHtml.hpp"

std::string PaginasHtml::Escapar( const std::string & texto ) {
   std::string r;
   for ( char c : texto ) {
      switch ( c ) {
         case '&':  r += "&amp;";  break;
         case '<':  r += "&lt;";   break;
         case '>':  r += "&gt;";   break;
         case '"':  r += "&quot;"; break;
         default:   r += c;
      }
   }
   return r;
}

std::string PaginasHtml::CodificarUrl( const std::string & texto ) {
   std::ostringstream out;
   for ( unsigned char c : texto ) {
      if ( isalnum( c ) || c == '-' || c == '_' || c == '.' || c == '~' ) {
         out << c;
      } else {
         out << '%' << std::uppercase << std::hex << std::setw( 2 ) << std::setfill( '0' ) << (int)c;
      }
   }
   return out.str();
}

// con punto decimal porque el cliente lo lee con std::stod
std::string PaginasHtml::Decimal( double valor ) {
   char buf[64];
   snprintf( buf, sizeof( buf ), "%.2f", valor );
   return buf;
}

std::string PaginasHtml::Inicio( const std::string & titulo ) {
   return "<!DOCTYPE html>\n<HTML>\n<HEAD><meta charset=\"utf-8\"><TITLE>"+ Escapar( titulo ) + "</TITLE></HEAD>\n<BODY>\n<H1>" + Escapar( titulo ) + "</H1>\n";
}

std::string PaginasHtml::Fin() {
   return "</BODY>\n</HTML>\n";
}

std::string PaginasHtml::PaginaCategorias( const std::vector<std::string> & categorias ) {
   std::string html = Inicio( "TicAmazon - Categorias" );
   html += "<UL>\n";
   for ( const std::string & c : categorias ) {
      html += "<LI><A href=\"/TicAmazon/list.php?category=" + CodificarUrl( c ) + "\">"+ Escapar( c ) + "</A></LI>\n";
   }
   html += "</UL>\n";
   return html + Fin();
}

std::string PaginasHtml::PaginaProductos( const std::string & categoria,const std::vector<Producto> & productos ) {
   std::string html = Inicio( "TicAmazon - " + categoria );
   html += "<TABLE border=\"1\">\n";
   html += "<TR><TH>Intermediario</TH><TH>Bodega</TH><TH>Categoria</TH>""<TH>Descripcion</TH><TH>Cantidad</TH><TH>Precio</TH></TR>\n";

   for ( const Producto & p : productos ) {
      html += "<TR><TD></TD>";
      html += "<TD>" + Escapar( p.Bodega() ) + "</TD>";
      html += "<TD>" + Escapar( p.Categoria() ) + "</TD>";
      html += "<TD>" + Escapar( p.Descripcion() ) + "</TD>";
      html += "<TD>" + std::to_string( p.Cantidad() ) + "</TD>";
      html += "<TD>" + Decimal( p.Precio() ) + "</TD></TR>\n";
   }

   html += "</TABLE>\n";
   return html + Fin();
}

std::string PaginasHtml::PaginaProforma( const Proforma & pf ) {
   std::string html = Inicio( "TicAmazon - Factura proforma" );

   if ( !pf.EsValida() ) {
      html += "<P>No se pudo generar la proforma:</P>\n<UL>\n";
      for ( const std::string & e : pf.Errores() ) {
         html += "<LI>" + Escapar( e ) + "</LI>\n";
      }
      html += "</UL>\n";
      return html + Fin();
   }

   html += "<TABLE border=\"1\">\n";
   html += "<TR><TH>Bodega</TH><TH>Categoria</TH><TH>Descripcion</TH>""<TH>Cantidad</TH><TH>Precio</TH><TH>Subtotal</TH></TR>\n";
   for ( const LineaProforma & l : pf.Lineas() ) {
      html += "<TR><TD>" + Escapar( l.bodega ) + "</TD>";
      html += "<TD>" + Escapar( l.categoria ) + "</TD>";
      html += "<TD>" + Escapar( l.descripcion ) + "</TD>";
      html += "<TD>" + std::to_string( l.cantidad ) + "</TD>";
      html += "<TD>" + Decimal( l.precioUnitario ) + "</TD>";
      html += "<TD>" + Decimal( l.Subtotal() ) + "</TD></TR>\n";
   }
   html += "</TABLE>\n<P><B>TOTAL: " + Decimal( pf.Total() ) + "</B></P>\n";
   return html + Fin();
}

std::string PaginasHtml::PaginaError( int codigo, const std::string & detalle ) {
   return Inicio( "Error " + std::to_string( codigo ) ) + "<P>" + Escapar( detalle ) + "</P>\n" + Fin();
}


std::string PaginasHtml::PaginaMensaje( const std::string & titulo, const std::string & texto ) {
   return Inicio( titulo ) + "<P>" + Escapar( texto ) + "</P>\n" + Fin();
}

std::string PaginasHtml::PaginaServidores( const std::vector<InfoServidor> & servidores ) {
   std::string html = Inicio( "TicAmazon - Servidores registrados" );
   html += "<TABLE border=\"1\">\n";
   html += "<TR><TH>Nombre</TH><TH>Direccion</TH><TH>Estado</TH><TH>Categorias</TH></TR>\n";
   for ( const InfoServidor & s : servidores ) {
      std::string cats;
      for ( size_t i = 0; i < s.categorias.size(); i++ ) {
         if ( i > 0 ) cats += ", ";
         cats += s.categorias[i];
      }
      html += "<TR><TD>" + Escapar( s.nombre ) + "</TD>";
      html += "<TD>" + Escapar( s.host ) + ":" + std::to_string( s.puerto ) + "</TD>";
      html += std::string( "<TD>" ) + ( s.activo ? "ACTIVO" : "INACTIVO" ) + "</TD>";
      html += "<TD>" + Escapar( cats ) + "</TD></TR>\n";
   }
   html += "</TABLE>\n";
   return html + Fin();
}

std::string PaginasHtml::PaginaRutas( const std::vector<std::pair<std::string, std::vector<std::string>>> & rutas ) {
   std::string html = Inicio( "TicAmazon - Tabla de rutas" );
   html += "<TABLE border=\"1\">\n<TR><TH>Categoria</TH><TH>Servidores</TH></TR>\n";
   for ( const auto & r : rutas ) {
      std::string srv;
      for ( size_t i = 0; i < r.second.size(); i++ ) {
         if ( i > 0 ) srv += ", ";
         srv += r.second[i];
      }
      html += "<TR><TD>" + Escapar( r.first ) + "</TD><TD>" + Escapar( srv ) + "</TD></TR>\n";
   }
   html += "</TABLE>\n";
   return html + Fin();
}


std::string PaginasHtml::PaginaProductosDistribuidos( const std::string & categoria, const std::vector<FilaProducto> & filas ) {
   std::string html = Inicio( "TicAmazon - " + categoria );
   html += "<TABLE border=\"1\">\n";
   html += "<TR><TH>Intermediario</TH><TH>Bodega</TH><TH>Categoria</TH><TH>Descripcion</TH><TH>Cantidad</TH><TH>Precio</TH></TR>\n";
   for ( const FilaProducto & f : filas ) {
      html += "<TR><TD>" + Escapar( f.intermediario ) + "</TD>";
      html += "<TD>" + Escapar( f.bodega ) + "</TD>";
      html += "<TD>" + Escapar( f.categoria ) + "</TD>";
      html += "<TD>" + Escapar( f.descripcion ) + "</TD>";
      html += "<TD>" + Escapar( f.cantidad ) + "</TD>";
      html += "<TD>" + Escapar( f.precio ) + "</TD></TR>\n";
   }
   html += "</TABLE>\n";
   return html + Fin();
}

std::string PaginasHtml::PaginaBitacora( const std::vector<EventoBitacora> & eventos,const ResumenBitacora & resumen, const std::string & filtroTipo ) {
   std::string html = Inicio( "TicAmazon - Bitacora" );
   html += "<SCRIPT>setTimeout(function(){location.reload();},3000);</SCRIPT>\n";
   html += "<P>Total de eventos registrados: <B>" + std::to_string( resumen.total ) + "</B>";
   if ( !filtroTipo.empty() ) {
      html += " | Filtro: <B>" + Escapar( filtroTipo ) + "</B>";
   }
   html += " | <A href=\"/TicAmazon/bitacora\">Ver todos</A></P>\n";
   html += "<TABLE border=\"1\">\n<TR><TH>Tipo</TH><TH>Cantidad</TH></TR>\n";
   for ( const auto & par : resumen.porTipo ) {
      html += "<TR><TD><A href=\"/TicAmazon/bitacora?tipo=" + CodificarUrl( par.first ) + "\">" + Escapar( par.first ) + "</A></TD><TD>" + std::to_string( par.second ) + "</TD></TR>\n";
   }
   html += "</TABLE>\n<BR>\n";
   html += "<TABLE border=\"1\">\n<TR><TH>Origen</TH><TH>Cantidad</TH></TR>\n";
   for ( const auto & par : resumen.porOrigen ) {
      html += "<TR><TD>" + Escapar( par.first ) + "</TD><TD>" + std::to_string( par.second ) + "</TD></TR>\n";
   }
   html += "</TABLE>\n<BR>\n";
   html += "<TABLE border=\"1\">\n<TR><TH>#</TH><TH>Fecha</TH><TH>Tipo</TH><TH>Origen</TH><TH>Mensaje</TH></TR>\n";
   for ( const EventoBitacora & e : eventos ) {
      html += "<TR><TD>" + std::to_string( e.id ) + "</TD>";
      html += "<TD>" + Escapar( e.fecha ) + "</TD>";
      html += "<TD>" + Escapar( e.tipo ) + "</TD>";
      html += "<TD>" + Escapar( e.origen ) + "</TD>";
      html += "<TD>" + Escapar( e.mensaje ) + "</TD></TR>\n";
   }
   html += "</TABLE>\n";
   return html + Fin();
}