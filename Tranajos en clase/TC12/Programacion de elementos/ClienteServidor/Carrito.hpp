#ifndef Carrito_hpp
#define Carrito_hpp

#include <vector>
#include "Producto.hpp"

struct ItemCarrito {
    Producto producto;
    int cantidadElegida = 0;
};

class Carrito {
public:
    void Agregar( const Producto & p, int cantidad );
    double Total() const;
    void MostrarFactura() const;

    //Arma el body de la proforma que se manda al servidor (POST /TicAmazon/proforma.php).
    const std::vector<ItemCarrito> & Items() const { return items; }

private:
    std::vector<ItemCarrito> items;
};

#endif