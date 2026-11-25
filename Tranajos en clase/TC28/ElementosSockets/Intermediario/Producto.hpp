#ifndef Producto_h
#define Producto_h

#include <string>

class Producto {
   public:
      Producto() : cantidad( 0 ), precio( 0.0 ) {}
      Producto( const std::string & bodega,const std::string & categoria, const std::string & descripcion,int cantidad, double precio ): bodega( bodega ), categoria( categoria ),descripcion( descripcion ), cantidad( cantidad ), precio( precio ) {}
      const std::string & Bodega() const { return bodega; }
      const std::string & Categoria() const { return categoria; }
      const std::string & Descripcion() const { return descripcion; }
      int Cantidad() const { return cantidad; }
      double Precio() const { return precio; }

   private:
      std::string bodega;
      std::string categoria;
      std::string descripcion;
      int cantidad;
      double precio;
};

#endif