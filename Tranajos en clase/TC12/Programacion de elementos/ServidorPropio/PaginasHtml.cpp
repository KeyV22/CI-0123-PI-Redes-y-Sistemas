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

// Mismo formato que espera parsearProductos del cliente qu es una fila de <TH>, y luego filas <TR><TD>..</TD>x6</TR> en el orden de intermediario, bodega, categoria, descripcion, cantidad, precio
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