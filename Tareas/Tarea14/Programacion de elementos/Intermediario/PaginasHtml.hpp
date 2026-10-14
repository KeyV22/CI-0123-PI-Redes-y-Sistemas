#ifndef PaginasHtml_hpp
#define PaginasHtml_hpp
#include <vector>
#include <string>
#include "Producto.hpp"
#include "Proforma.hpp"

// Genera el HTML de las respuestas pero sin logica de red (una representacion de lo que devolvia el del porfe)
class PaginasHtml {
   public:
      static std::string PaginaCategorias( const std::vector<std::string> & categorias );
      static std::string PaginaProductos( const std::string & categoria,const std::vector<Producto> & productos );
      static std::string PaginaProforma( const Proforma & pf );
      static std::string PaginaError( int codigo, const std::string & detalle );

      static std::string Escapar( const std::string & texto );
      static std::string CodificarUrl( const std::string & texto );
      static std::string Decimal( double valor );

   private:
      static std::string Inicio( const std::string & titulo );
      static std::string Fin();
};

#endif