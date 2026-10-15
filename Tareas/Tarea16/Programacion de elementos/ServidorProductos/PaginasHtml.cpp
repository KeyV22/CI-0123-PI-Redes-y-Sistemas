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

static const char * ESTILO =
   "<style>\n"
   "form.salir{position:absolute;right:18px;bottom:7px;margin:0;z-index:2}\n"
   "form.salir button{background:#8f2d2d;color:#fff;border:0;border-radius:6px;padding:7px 16px;font-size:.85em;cursor:pointer;letter-spacing:1px}\n"
   "form.salir button:hover{background:#b33a3a}\n"
   "html{background:#e8f0ea}\n"
   "body{font-family:Segoe UI,Arial,sans-serif;color:#1d2b24;max-width:900px;margin:0 auto;padding:24px 24px 70px;"
        "background-image:radial-gradient(#c5d9cb 1.5px,transparent 1.5px);background-size:22px 22px;min-height:100vh;"
        "border-left:6px solid #1f5c3f;border-right:6px solid #1f5c3f;background-color:#f1f6f2;position:relative}\n"
   "h1{background:linear-gradient(135deg,#14452e,#1f5c3f);color:#fff;margin:0 0 22px;padding:18px 22px;border-radius:10px;"
        "font-size:1.5em;border-bottom:5px solid #0b2a1b;box-shadow:0 3px 8px rgba(0,0,0,.25)}\n"
   "h1::before{content:'\\1F6D2  ';}\n"
   "h1::after{content:'\\2726 \\2726 \\2726';float:right;font-size:.7em;letter-spacing:6px;opacity:.55;margin-top:4px}\n"
   "table{width:100%;border-collapse:collapse;background:#fff;border-radius:10px;overflow:hidden;"
        "box-shadow:0 2px 6px rgba(0,0,0,.18);border-top:4px solid #1f5c3f}\n"
   "th{background:#2d6a4f;color:#fff;text-align:left;padding:10px 12px}\n"
   "td{padding:9px 12px;border-bottom:1px solid #dde8e0}\n"
   "tr:nth-child(even) td{background:#f1f7f3}\n"
   "tr:hover td{background:#d8eadf}\n"
   "td:nth-child(5),td:nth-child(6),th:nth-child(5),th:nth-child(6){text-align:right}\n"
   "ul{list-style:none;padding:0;display:grid;grid-template-columns:repeat(auto-fill,minmax(180px,1fr));gap:14px}\n"
   "li a{display:block;background:#fff;padding:20px 14px;border-radius:12px;text-align:center;text-decoration:none;"
        "color:#14452e;font-weight:600;border:2px solid #b7d3c1;box-shadow:0 2px 5px rgba(0,0,0,.12);transition:all .15s}\n"
   "li a::before{content:'\\1F33F';display:block;font-size:1.8em;margin-bottom:6px}\n"
   "li a[href$='=Alimentos']::before{content:'\\1F35E'}\n"
   "li a[href$='=Bebidas']::before{content:'\\1F964'}\n"
   "li a[href$='=Salud']::before{content:'\\1F480'}\n"
   "li a[href$='=Electronica']::before{content:'\\1F4F1'}\n"
   "li a[href$='=Hogar']::before{content:'\\1F343'}\n"
   "li a:hover{background:#1f5c3f;color:#fff;border-color:#0b2a1b;transform:translateY(-3px)}\n"
   "a.volver{display:inline-block;margin-bottom:14px;color:#1f5c3f;font-weight:600;text-decoration:none}\n"
   "a.volver:hover{text-decoration:underline}\n"
   "p{font-size:1.1em;background:#fff;border-left:5px solid #1f5c3f;padding:10px 14px;border-radius:6px}\n"
   "body::after{content:'TicAmazon  \\2022  Tienda en linea';position:absolute;left:0;right:0;bottom:0;height:44px;line-height:44px;"
        "text-align:center;background:#14452e;color:#cfe5d6;font-size:.85em;letter-spacing:2px}\n"
   "</style>\n";
   

std::string PaginasHtml::Inicio( const std::string & titulo ) {
   return std::string( "<!DOCTYPE html>\n<HTML>\n<HEAD><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><TITLE>" )
        + Escapar( titulo ) + "</TITLE>\n" + ESTILO + "</HEAD>\n<BODY>\n<H1>" + Escapar( titulo ) + "</H1>\n";
}

std::string PaginasHtml::Fin() {
   return "<form class=\"salir\" method=\"post\" action=\"/TicAmazon/salir\" onsubmit=\"return confirm('Cerrar el servidor?')\">"
          "<button type=\"submit\">&#x23FB; Salir</button></form>\n</BODY>\n</HTML>\n";
}

std::string PaginasHtml::PaginaCerrado() {
   return Inicio( "TicAmazon - Servidor cerrado" ) + "<P>El servidor se ha cerrado. Ya puede cerrar esta ventana.</P>\n</BODY>\n</HTML>\n";
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
   html += "<A class=\"volver\" href=\"/TicAmazon/list.php\">&larr; Volver a categorias</A>\n";
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