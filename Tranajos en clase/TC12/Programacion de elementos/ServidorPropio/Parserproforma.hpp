#ifndef ParserProforma_hpp
#define ParserProforma_hpp

#include <string>
#include <vector>
#include "Proforma.hpp"   // define SolicitudItem

// Convierte el body de POST /TicAmazon/proforma.php en el vector de
// SolicitudItem que espera Inventario::GenerarProforma.
//
//
//   Content-Type: application/x-www-form-urlencoded
//   Body: item0_categoria=Electronica&item0_producto=Pantalla&item0_cantidad=2
//        &item1_categoria=Alimentos+y+bebidas&item1_producto=Banano&item1_cantidad=5
//
// Es decir, cada linea del carrito va indexada con el prefijo "itemN_"
// (N = 0, 1, 2, ...) y trae tres campos: categoria, producto, cantidad.
class ParserProforma {
   public:
      static std::vector<SolicitudItem> Parsear( const std::string & cuerpo );
};

#endif