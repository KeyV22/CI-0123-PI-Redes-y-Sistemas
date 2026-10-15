#include "Producto.hpp"

static std::string extraerCelda( const std::string & html, size_t & pos ) {
   size_t inicioTag = html.find(">", html.find("<TD", pos));
   size_t fin = html.find("</TD>", inicioTag);
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
      prod.intermediario = extraerCelda(body, p);
      prod.bodega        = extraerCelda(body, p);
      prod.categoria     = extraerCelda(body, p);
      prod.descripcion   = extraerCelda(body, p);
      prod.cantidad      = std::stoi(extraerCelda(body, p));
      prod.precio        = std::stod(extraerCelda(body, p));

      productos.push_back(prod);
      pos = finFila + 5;
   }

   return productos;
}   