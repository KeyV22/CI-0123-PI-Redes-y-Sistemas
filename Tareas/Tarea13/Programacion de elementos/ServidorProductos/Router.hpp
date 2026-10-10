#ifndef Router_hpp
#define Router_hpp

#include "Httprequest.hpp"
#include "ServicioProductos.hpp"

// Traduce un HttpRequest ya parseado en la llamada correcta a
// ServicioProductos. Es el unico lugar donde se definen las rutas
// que expone el servidor.
class Router {
   public:
      // Rutas que expone el servidor :
      //   GET  /TicAmazon/list.php                    -> listar categorias
      //   GET  /TicAmazon/list.php?category=X          -> listar productos de X
      //   POST /TicAmazon/proforma.php                 -> generar proforma (commit 5)
      static RespuestaServicio Despachar( const HttpRequest & req, ServicioProductos & servicio );

   private:
      static const std::string RUTA_LISTADO;
      static const std::string RUTA_PROFORMA;
      static const std::string RUTA_SALIR;
};

#endif