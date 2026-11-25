#ifndef Producto_hpp
#define Producto_hpp

#include <string>
#include <vector>


struct Producto {
    std::string intermediario;
    std::string bodega;
    std::string categoria;
    std::string descripcion;
    int cantidad = 0;
    double precio = 0.0;
};


std::vector<Producto> parsearProductos( const std::string & body );

#endif