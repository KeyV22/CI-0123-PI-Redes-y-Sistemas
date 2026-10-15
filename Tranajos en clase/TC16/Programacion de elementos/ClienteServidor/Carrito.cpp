#include <cstdio>
#include "Carrito.hpp"

void Carrito::Agregar( const Producto & p, int cantidad ) {
   ItemCarrito item;
   item.producto = p;
   item.cantidadElegida = cantidad;
   items.push_back(item);
}

double Carrito::Total() const {
   double total = 0.0;
   for ( const auto & item : items ) {
      total += item.producto.precio * item.cantidadElegida;
   }
   return total;
}

void Carrito::MostrarFactura() const {
   printf("\nFACTURA\n");
   printf("%-20s %8s %10s %10s\n", "Producto", "Cant.", "Precio", "Subtotal");

   for ( const auto & item : items ) {
      double subtotal = item.producto.precio * item.cantidadElegida;
      printf("%-20s %8d %10.2f %10.2f\n",item.producto.descripcion.c_str(),item.cantidadElegida,item.producto.precio,subtotal);
   }

   printf("TOTAL: %.2f\n", Total());
}