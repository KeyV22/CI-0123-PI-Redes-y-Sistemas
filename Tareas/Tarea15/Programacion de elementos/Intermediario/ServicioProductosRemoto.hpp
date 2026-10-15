#ifndef ServicioProductosRemoto_hpp
#define ServicioProductosRemoto_hpp

#include <string>
#include <vector>
#include "ServicioProductos.hpp"

// Implementacion de ServicioProductos que NO toca el almacenamiento:
// por cada llamada abre una conexion TCP hacia el ServidorProductos,
// le manda el mismo HTTP que ya entiende y devuelve su respuesta.
class ServicioProductosRemoto : public ServicioProductos {
   public:
      ServicioProductosRemoto( const std::string & host, int puerto )
         : host( host ), puerto( puerto ) {}

      RespuestaServicio ListarCategorias() const override;
      RespuestaServicio ListarProductos( const std::string & categoria ) const override;
      RespuestaServicio GenerarProforma( const std::vector<SolicitudItem> & items ) const override;

   private:
      // Manda un request al ServidorProductos y arma la RespuestaServicio.
      // Si no se puede conectar devuelve un 500 con pagina de error.
      RespuestaServicio Reenviar( const std::string & metodo, const std::string & ruta,
                                  const std::string & cuerpo ) const;

      static std::string CodificarUrl( const std::string & texto );

      std::string host;
      int puerto;
};

#endif