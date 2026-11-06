#ifndef PaginasHtml_hpp
#define PaginasHtml_hpp
#include <vector>
#include <string>
#include "Producto.hpp"
#include "Proforma.hpp"
#include <utility>
#include "RegistroServidores.hpp"
#include "Bitacora.hpp"


struct FilaProducto {
   std::string intermediario;   // nombre del servidor de origen
   std::string bodega;
   std::string categoria;
   std::string descripcion;
   std::string cantidad;
   std::string precio;
};

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

      static std::string PaginaMensaje( const std::string & titulo, const std::string & texto );
      static std::string PaginaServidores( const std::vector<InfoServidor> & servidores );
      static std::string PaginaRutas( const std::vector<std::pair<std::string, std::vector<std::string>>> & rutas );
      static std::string PaginaProductosDistribuidos( const std::string & categoria, const std::vector<FilaProducto> & filas );
      static std::string PaginaBitacora( const std::vector<EventoBitacora> & eventos,const ResumenBitacora & resumen, const std::string & filtroTipo );

   private:
      static std::string Inicio( const std::string & titulo );
      static std::string Fin();
};

#endif