#include "Producto.hpp"
#include <stdexcept>

static std::string extraerCelda( const std::string & html, size_t & pos, size_t finFila ) {
   size_t inicioTd = html.find(">", html.find("<TD", pos));
   if ( inicioTd == std::string::npos || inicioTd >= finFila )
      throw std::runtime_error( "parsearProductos - fila incompleta" );
   size_t inicioTag = html.find( ">", inicioTd );
   size_t fin = html.find( "</TD>", inicioTag );
   if ( inicioTag == std::string::npos || fin == std::string::npos ) {
      throw std::runtime_error( "parsearProductos - celda sin cerrar" );
   }
   pos = fin + 5; 

   std::string valor = html.substr(inicioTag + 1, fin - inicioTag - 1);
   size_t a = valor.find_first_not_of(" \r\n\t");
   size_t b = valor.find_last_not_of(" \r\n\t");
   if ( a == std::string::npos ) return "";
   return valor.substr(a, b - a + 1);
}

std::vector<Producto> parsearProductos( const std::string & body ) {
   std::vector<Producto> productos;
   size_t pos = body.find("<TR>", body.find("</TH>"));

   while ( (pos = body.find("<TR>", pos)) != std::string::npos ) {
      size_t finFila = body.find("</TR>", pos);
      if ( finFila == std::string::npos ) break;

      size_t p = pos;
      Producto prod;
      prod.intermediario = extraerCelda(body, p, finFila);
      prod.bodega        = extraerCelda(body, p, finFila);
      prod.categoria     = extraerCelda(body, p, finFila);
      prod.descripcion   = extraerCelda(body, p, finFila);
      prod.cantidad      = std::stoi(extraerCelda(body, p, finFila));
      prod.precio        = std::stod(extraerCelda(body, p, finFila));

      productos.push_back(prod);
      pos = finFila + 5;
   }

   return productos;
}   