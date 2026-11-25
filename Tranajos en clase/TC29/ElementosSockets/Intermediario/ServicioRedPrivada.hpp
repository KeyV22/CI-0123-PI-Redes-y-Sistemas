#ifndef ServicioRedPrivada_hpp
#define ServicioRedPrivada_hpp

#include "ServicioProductos.hpp"

// Lo que se expone en la interfaz PRIVADA del intermediario: solo el registro y el
// desregistro de servidores de productos. Todo lo demas va por la red publica.
class ServicioRedPrivada : public ServicioProductos {
   public:
      explicit ServicioRedPrivada( ServicioProductos & real ) : real( real ) {}

      RespuestaServicio ListarCategorias() const override { return Prohibido(); }
      RespuestaServicio ListarProductos( const std::string & ) const override { return Prohibido(); }
      RespuestaServicio GenerarProforma( const std::vector<SolicitudItem> & ) const override { return Prohibido(); }
      RespuestaServicio ListarServidores() const override { return Prohibido(); }
      RespuestaServicio ListarRutas() const override { return Prohibido(); }
      RespuestaServicio ListarBitacora( const std::string & ) const override { return Prohibido(); }

      RespuestaServicio RegistrarServidor( const std::string & nombre, const std::string & host, int puerto,
                                           const std::vector<std::string> & categorias ) const override {
         return real.RegistrarServidor( nombre, host, puerto, categorias );
      }
      RespuestaServicio DesregistrarServidor( const std::string & host, int puerto ) const override {
         return real.DesregistrarServidor( host, puerto );
      }

   private:
      RespuestaServicio Prohibido() const {
         return Error( 403, "Esta interfaz es solo para servidores de productos; use la red publica" );
      }
      ServicioProductos & real;
};

#endif