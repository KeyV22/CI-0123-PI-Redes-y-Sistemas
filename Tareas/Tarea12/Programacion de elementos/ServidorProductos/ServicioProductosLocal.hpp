#ifndef ServicioProductosLocal_hpp
#define ServicioProductosLocal_hpp

#include <string>
#include <vector>
#include "ServicioProductos.hpp"
#include "Inventario.hpp"

class ServicioProductosLocal : public ServicioProductos {
   public:
      explicit ServicioProductosLocal( Inventario & inventario ) : inventario( inventario ) {}

      RespuestaServicio ListarCategorias() const override;
      RespuestaServicio ListarProductos( const std::string & categoria ) const override;
      RespuestaServicio GenerarProforma( const std::vector<SolicitudItem> & items ) const override;

   private:
      Inventario & inventario;
};

#endif