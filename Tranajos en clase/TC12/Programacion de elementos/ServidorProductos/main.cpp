#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include "almacenamiento.hpp"
#include "Inventario.hpp"
#include "ServicioProductosLocal.hpp"
#include "ServicioProductos.hpp"
#include "Servidor.hpp"

// Valida el registro antes de insertarlo "bodega:categoria:producto:cantidad:precio")
static bool Insertar( Almacenamiento & almacen, const std::string & bodega,const std::string & categoria, const std::string & producto,const std::string & cantidad, const std::string & precio ) {
   for ( const std::string * campo : { &bodega, &categoria, &producto, &cantidad, &precio } ) {
      if ( campo->find_first_of( ":\n" ) != std::string::npos ) {
         std::cerr << "Campo con ':' o salto de linea no permitido: " << *campo << "\n";
         return false;
      }
   }

   // bodega:categoria:producto:cantidad:precio
   size_t largo = bodega.size() + categoria.size() + producto.size()+ cantidad.size() + precio.size() + 5;
   if ( largo > (size_t)TAM_TEXTO_DATOS ) {
      std::cerr << "Registro de " << largo << " bytes excede " << TAM_TEXTO_DATOS<< ": " << producto << "\n";
      return false;
   }

   return almacen.insertar_producto( bodega, categoria, producto, cantidad, precio );
}

// Crea el archivo desde cero y lo siembra con dos bodegas de prueba
static bool CrearYSembrar( Almacenamiento & almacen, const std::string & ruta ) {
   if ( !almacen.crear_archivo( ruta ) ) {
      std::cerr << "No se pudo crear " << ruta << "\n";
      return false;
   }

   almacen.crear_bodega( "Bodega-1" );
   Insertar( almacen, "Bodega-1", "Alimentos y bebidas", "banano", "100", "10.00" );
   Insertar( almacen, "Bodega-1", "Alimentos y bebidas", "manzana", "10", "300.00" );
   Insertar( almacen, "Bodega-1", "Salud y belleza", "perfume", "33", "44.00" );

   almacen.crear_bodega( "Bodega-2" );
   Insertar( almacen, "Bodega-2", "Salud y belleza", "shampoo", "22", "11.00" );
   Insertar( almacen, "Bodega-2", "Alimentos y bebidas", "arroz", "50", "15.00" );
   Insertar( almacen, "Bodega-2", "Electronica", "pantalla", "5", "150.00" );

   std::cout << "Contenedor creado: " << ruta << " con 2 bodegas.\n";
   return true;
}

int main( int argc, char * argv[] ) {
   int puerto = ( argc > 1 ) ? std::atoi( argv[1] ) : 8080;
   std::string ruta = ( argc > 2 ) ? argv[2] : "bodegas.data";

   Almacenamiento almacen;

   std::ifstream existe( ruta );
   bool yaExiste = existe.good();
   existe.close();

   bool abierto = yaExiste ? almacen.abrir_archivo( ruta ) : CrearYSembrar( almacen, ruta );
   if ( !abierto ) {
      std::cerr << "No se pudo preparar " << ruta << "\n";
      return 1;
   }
   Inventario inventario( almacen );
   ServicioProductosLocal servicio( inventario );
   ServicioProductos servicio( inventario );

   Servidor servidor( puerto );
   if ( !servidor.Iniciar() ) {
      return 1;
   }
   servidor.Ejecutar( servicio );   

   almacen.cerrar_archivo();
   return 0;
}