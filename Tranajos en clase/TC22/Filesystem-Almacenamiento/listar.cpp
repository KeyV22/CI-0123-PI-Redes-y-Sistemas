// listar: muestra los productos guardados en un archivo de bodegas, agrupados por categoria.
//
// Uso:  ./listar [archivo] [categoria]
//   archivo    archivo de almacenamiento (por defecto bodegas.data)
//   categoria  si se da, solo muestra esa categoria (sin distinguir mayusculas)
//
// No modifica el archivo. NO USAR mientras un servidor de productos lo tiene abierto
// escribiendo. MUY AMABLES SE LOS AGRADEZCO DE TODO CORAZON
// IMPORTANTE no hagan make clean cuando corran esto. ME ACABO DE ECHAR LAS BODEGAS NO LO HAGAN
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "almacenamiento.hpp"

static std::string Minusculas( std::string s ) {
   std::transform( s.begin(), s.end(), s.begin(), []( unsigned char c ) { return std::tolower( c ); } );
   return s;
}

int main( int argc, char * argv[] ) {
   std::string ruta = ( argc > 1 ) ? argv[1] : "bodegas.data";
   std::string filtro = ( argc > 2 ) ? Minusculas( argv[2] ) : "";

   Almacenamiento almacen;
   if ( !almacen.abrir_archivo( ruta ) ) {
      std::cerr << "No se pudo abrir " << ruta << "\n";
      return 1;
   }

   // categoria -> productos (de todas las bodegas)
   std::map<std::string, std::vector<ProductoTexto>> porCategoria;
   std::vector<std::string> bodegas = almacen.listar_bodegas();
   size_t total = 0;
   for ( const std::string & b : bodegas ) {
      for ( const ProductoTexto & p : almacen.listar_productos( b ) ) {
         porCategoria[p.categoria].push_back( p );
         total++;
      }
   }
   almacen.cerrar_archivo();

   std::cout << "Archivo: " << ruta << "\n";
   std::cout << "Bodegas: " << bodegas.size() << "   Productos: " << total
             << "   Categorias: " << porCategoria.size() << "\n";

   bool mostro = false;
   for ( const auto & par : porCategoria ) {
      if ( !filtro.empty() && Minusculas( par.first ) != filtro ) continue;
      mostro = true;
      std::cout << "\n== Categoria: " << par.first << " (" << par.second.size() << " productos) ==\n";
      std::cout << std::left << "  " << std::setw( 12 ) << "Bodega" << std::setw( 28 ) << "Producto"
                << std::right << std::setw( 10 ) << "Cantidad" << std::setw( 12 ) << "Precio" << "\n";
      for ( const ProductoTexto & p : par.second ) {
         std::cout << std::left << "  " << std::setw( 12 ) << p.bodega << std::setw( 28 ) << p.producto
                   << std::right << std::setw( 10 ) << p.cantidad << std::setw( 12 ) << p.precio << "\n";
      }
   }
   if ( !mostro ) {
      std::cout << "\nNo hay productos" << ( filtro.empty() ? "" : " en la categoria '" + std::string( argv[2] ) + "'" ) << ".\n";
      return filtro.empty() ? 0 : 2;
   }

   // Resumen: en que bodegas esta cada categoria
   if ( filtro.empty() ) {
      std::cout << "\n== Resumen: categoria -> bodegas ==\n";
      for ( const auto & par : porCategoria ) {
         std::set<std::string> donde;
         for ( const ProductoTexto & p : par.second ) donde.insert( p.bodega );
         std::cout << "  " << par.first << ": ";
         bool primero = true;
         for ( const std::string & b : donde ) { std::cout << ( primero ? "" : ", " ) << b; primero = false; }
         std::cout << "\n";
      }
   }
   return 0;
}