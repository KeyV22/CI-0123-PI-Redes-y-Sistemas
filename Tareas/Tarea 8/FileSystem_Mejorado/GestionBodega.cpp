/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  TicAmazon - GestionBodega
  *
  *  Programa standalone (sin red, sin cliente/intermediario) para que el
  *  operador de la bodega administre el inventario directamente sobre el
  *  archivo bodega_ticamazon.dat: insertar productos nuevos, listar el
  *  catalogo, reabastecer/corregir stock, y eliminar (extraer) productos.
  *
 **/

#include <iostream>
#include <string>
#include <cstring>
#include <limits>

#include "FileSystem.hpp"

Producto CrearProducto( uint32_t id, const std::string & nombre, uint32_t precio, uint32_t stock ) {
   Producto p{};
   p.idProducto = id;
   p.precio = precio;
   p.cantidadDisponible = stock;
   memset( p.nombre, 0, 20 );
   strncpy( p.nombre, nombre.c_str(), 19 );
   return p;
}

uint32_t LeerEntero( const std::string & etiqueta ) {
   uint32_t valor;
   while ( true ) {
      std::cout << etiqueta;
      if ( std::cin >> valor ) {
         std::cin.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
         return valor;
      }
      std::cin.clear();
      std::cin.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
      std::cout << "  (valor invalido, intente de nuevo)\n";
   }
}

std::string LeerLinea( const std::string & etiqueta ) {
   std::cout << etiqueta;
   std::string valor;
   std::getline( std::cin, valor );
   return valor;
}


void MenuListarCategorias( FileSystem & fs ) {
   std::cout << "\nCategorias:\n";
   for ( auto & c : fs.listarCategorias() ) {
      std::cout << "  - " << c << "\n";
   }
}

void MenuListarProductos( FileSystem & fs ) {
   uint32_t idCategoria = LeerEntero( "Id numerico de la categoria: " );
   auto productos = fs.listarProductos( (uint8_t) idCategoria );
   if ( productos.empty() ) {
      std::cout << "(sin productos en esa categoria, o la categoria no existe)\n";
      return;
   }
   std::cout << "\nId\tNombre\t\t\tPrecio\tStock\n";
   for ( auto & p : productos ) {
      std::cout << p.idProducto << "\t" << p.nombre << "\t\t" << p.precio << "\t" << p.cantidadDisponible << "\n";
   }
}

void MenuInsertar( FileSystem & fs ) {
   uint32_t idCategoria = LeerEntero( "Id numerico de la categoria (nueva o existente): " );
   std::string nombreCategoria = LeerLinea( "Nombre de la categoria (max 15 caracteres): " );
   uint32_t idProducto = LeerEntero( "Id del producto nuevo: " );
   std::string nombreProducto = LeerLinea( "Nombre del producto (max 19 caracteres): " );
   uint32_t precio = LeerEntero( "Precio: " );
   uint32_t stock = LeerEntero( "Stock inicial: " );

   fs.agregarProducto( (uint8_t) idCategoria, nombreCategoria,
                        CrearProducto( idProducto, nombreProducto, precio, stock ) );

   std::cout << "OK: producto " << idProducto << " (" << nombreProducto << ") insertado en \""
             << nombreCategoria << "\".\n";
}

void MenuActualizarStock( FileSystem & fs ) {
   uint32_t idProducto = LeerEntero( "Id del producto: " );
   std::cout << "Cantidad a sumar (positivo = reabastecer, negativo = corregir/quitar): ";
   int32_t delta;
   std::cin >> delta;
   std::cin.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );

   std::string error;
   if ( fs.actualizarStock( idProducto, delta, &error ) ) {
      std::cout << "OK: stock actualizado.\n";
   } else if ( "404" == error ) {
      std::cout << "ERROR: no existe un producto con id " << idProducto << ".\n";
   } else {
      std::cout << "ERROR: " << error << " (el stock no puede quedar negativo).\n";
   }
}

void MenuEliminar( FileSystem & fs ) {
   uint32_t idProducto = LeerEntero( "Id del producto a eliminar (extraer del catalogo): " );

   std::string error;
   if ( fs.eliminarProducto( idProducto, &error ) ) {
      std::cout << "OK: producto " << idProducto << " eliminado de la bodega.\n";
   } else {
      std::cout << "ERROR: no existe un producto con id " << idProducto << ".\n";
   }
}


int main( int argc, char ** argv ) {

   std::string archivo = ( argc > 1 ) ? argv[1] : "bodega_ticamazon.dat";
   FileSystem fs( archivo );

   std::cout << "=== TicAmazon - Gestion de Bodega (" << archivo << ") ===\n";

   bool corriendo = true;
   while ( corriendo ) {

      std::cout << "\n1) Listar categorias\n"
                   "2) Listar productos de una categoria\n"
                   "3) Insertar producto nuevo\n"
                   "4) Actualizar stock (reabastecer / corregir)\n"
                   "5) Eliminar producto (extraer)\n"
                   "6) Ver estado del bitmap\n"
                   "7) Salir\n"
                   "Opcion: ";

      std::string opcion;
      if ( !std::getline( std::cin, opcion ) ) {
         std::cout << "\n(entrada terminada, saliendo)\n";
         break;
      }

      if ( "1" == opcion )      MenuListarCategorias( fs );
      else if ( "2" == opcion ) MenuListarProductos( fs );
      else if ( "3" == opcion ) MenuInsertar( fs );
      else if ( "4" == opcion ) MenuActualizarStock( fs );
      else if ( "5" == opcion ) MenuEliminar( fs );
      else if ( "6" == opcion ) fs.imprimirEstadoBitmap();
      else if ( "7" == opcion ) { std::cout << "Saliendo...\n"; corriendo = false; }
      else std::cout << "Opcion invalida.\n";

   }

   return 0;

}
