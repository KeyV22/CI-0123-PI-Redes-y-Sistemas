#ifndef ServicioProductos_hpp
#define ServicioProductos_hpp

#include <string>
#include <vector>
#include "Proforma.hpp"

// Resultado listo para que la capa de red arme la respuesta HTTP
struct RespuestaServicio {
   int codigo;                  // 200, 400, 404, 422...
   std::string frase;           // "OK", "Bad Request"...
   std::string tipoContenido;   // "text/html; charset=utf-8"
   std::string cuerpo;          // HTML
};

class ServicioProductos {
   public:
      virtual ~ServicioProductos() {}

      virtual RespuestaServicio ListarCategorias() const = 0;
      virtual RespuestaServicio ListarProductos( const std::string & categoria ) const = 0;
      virtual RespuestaServicio GenerarProforma( const std::vector<SolicitudItem> & items ) const = 0;

      // Para rutas desconocidas, metodos no soportados, parametros mal formados, etc.
      RespuestaServicio Error( int codigo, const std::string & detalle ) const;

   protected:
      static RespuestaServicio Construir( int codigo, const std::string & cuerpo );
      static std::string Frase( int codigo );
};

#endif