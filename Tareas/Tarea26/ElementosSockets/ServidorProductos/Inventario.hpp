#ifndef Inventario_hpp
#define Inventario_hpp

#include <mutex>
#include <string>
#include <vector>
#include "almacenamiento.hpp"
#include "Proforma.hpp"
#include "Producto.hpp"

class Inventario {
   public:
      explicit Inventario( Almacenamiento & almacen );

      std::vector<std::string> ListarCategorias() const;
      std::vector<Producto> ListarProductos( const std::string & categoria ) const;

      // Valida existencias y calcula totales pero no modifica el inventario
      Proforma GenerarProforma( const std::vector<SolicitudItem> & items ) const;

   private:
      std::vector<Producto> LeerTodoSinBloqueo() const;

      Almacenamiento & almacen;
      mutable std::mutex mtx;
};

#endif