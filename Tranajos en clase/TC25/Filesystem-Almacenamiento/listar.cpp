// listar: lista el contenido de un archivo de bodegas (agrupado por categoria) y
// extrae una categoria elegida.
//
// Uso:
//   ./listar [archivo]                              lista todo, agrupado por categoria
//   ./listar [archivo] <categoria>                  lista solo esa categoria
//   ./listar [archivo] --extraer <categoria>        copia la categoria a <categoria>.txt
//   ./listar [archivo] --extraer <categoria> --quitar
//                                                   ademas la saca del sistema de archivos
//                                                   (antes hace una copia <archivo>.bak)
//
// El archivo de salida tiene una linea por producto: bodega|categoria|producto|cantidad|precio
// NO USAR mientras un servidor de productos tiene el archivo abierto.
// IMPORTANTE: no hagan make clean cuando corran esto: borra bodegas.data.
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
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

static std::string NombreArchivo( std::string categoria ) {
   for ( char & c : categoria ) {
      if ( !std::isalnum( static_cast<unsigned char>( c ) ) ) c = '_';
   }
   return categoria + ".txt";
}

static void ImprimirCategoria( const std::string & nombre, const std::vector<ProductoTexto> & productos ) {
   std::cout << "\n== Categoria: " << nombre << " (" << productos.size() << " productos) ==\n";
   std::cout << std::left << "  " << std::setw( 12 ) << "Bodega" << std::setw( 28 ) << "Producto"
             << std::right << std::setw( 10 ) << "Cantidad" << std::setw( 12 ) << "Precio" << "\n";
   for ( const ProductoTexto & p : productos ) {
      std::cout << std::left << "  " << std::setw( 12 ) << p.bodega << std::setw( 28 ) << p.producto
                << std::right << std::setw( 10 ) << p.cantidad << std::setw( 12 ) << p.precio << "\n";
   }
}

int main( int argc, char * argv[] ) {
   std::vector<std::string> pos;
   std::string extraer;
   bool quitar = false;
   for ( int i = 1; i < argc; i++ ) {
      std::string a = argv[i];
      if ( a == "--extraer" ) {
         if ( i + 1 >= argc ) { std::cerr << "--extraer necesita una categoria\n"; return 1; }
         extraer = argv[++i];
      } else if ( a == "--quitar" ) {
         quitar = true;
      } else {
         pos.push_back( a );
      }
   }
   if ( quitar && extraer.empty() ) { std::cerr << "--quitar solo se usa junto con --extraer <categoria>\n"; return 1; }

   std::string ruta = "bodegas.data";
   std::string filtro;
   if ( !pos.empty() ) {
      if ( std::filesystem::exists( pos[0] ) ) {
         ruta = pos[0];
         if ( pos.size() > 1 ) filtro = pos[1];
      } else {
         filtro = pos[0];   // el primer argumento no es un archivo: es una categoria
      }
   }
   if ( !extraer.empty() ) filtro = extraer;
   std::string filtroMin = Minusculas( filtro );

   Almacenamiento almacen;
   if ( !almacen.abrir_archivo( ruta ) ) {
      std::cerr << "No se pudo abrir " << ruta << "\n";
      return 1;
   }

   std::map<std::string, std::vector<ProductoTexto>> porCategoria;
   std::vector<std::string> bodegas = almacen.listar_bodegas();
   size_t total = 0;
   for ( const std::string & b : bodegas ) {
      for ( const ProductoTexto & p : almacen.listar_productos( b ) ) {
         porCategoria[p.categoria].push_back( p );
         total++;
      }
   }

   std::cout << "Archivo: " << ruta << "\n";
   std::cout << "Bodegas: " << bodegas.size() << "   Productos: " << total
             << "   Categorias: " << porCategoria.size() << "\n";

   // ---- Extraer una categoria ----
   if ( !extraer.empty() ) {
      for ( const auto & par : porCategoria ) {
         if ( Minusculas( par.first ) != filtroMin ) continue;

         ImprimirCategoria( par.first, par.second );

         std::string salida = NombreArchivo( par.first );
         std::ofstream out( salida );
         if ( !out ) { std::cerr << "No se pudo crear " << salida << "\n"; almacen.cerrar_archivo(); return 1; }
         for ( const ProductoTexto & p : par.second ) {
            out << p.bodega << '|' << p.categoria << '|' << p.producto << '|' << p.cantidad << '|' << p.precio << "\n";
         }
         out.close();
         std::cout << "\nExtraida la categoria '" << par.first << "': " << par.second.size()
                   << " productos copiados a " << salida << "\n";

         if ( quitar ) {
            std::cout << "Se van a QUITAR esos " << par.second.size() << " productos de " << ruta
                      << " (copia de seguridad en " << ruta << ".bak). Continuar? (s/n): ";
            std::string r;
            std::getline( std::cin, r );
            if ( r != "s" && r != "S" ) {
               std::cout << "Cancelado: el archivo no se modifico.\n";
               almacen.cerrar_archivo();
               return 0;
            }
            almacen.cerrar_archivo();
            std::filesystem::copy_file( ruta, ruta + ".bak", std::filesystem::copy_options::overwrite_existing );
            if ( !almacen.abrir_archivo( ruta ) ) { std::cerr << "No se pudo reabrir " << ruta << "\n"; return 1; }
            size_t quitados = 0;
            for ( const ProductoTexto & p : par.second ) {
               if ( almacen.extraer_producto( p.bodega, p.producto ) ) quitados++;
            }
            std::cout << "Quitados del sistema de archivos: " << quitados << " de " << par.second.size() << "\n";
         }
         almacen.cerrar_archivo();
         return 0;
      }
      std::cout << "\nNo existe la categoria '" << extraer << "'.\n";
      almacen.cerrar_archivo();
      return 2;
   }

   almacen.cerrar_archivo();

   // ---- Listado ----
   bool mostro = false;
   for ( const auto & par : porCategoria ) {
      if ( !filtro.empty() && Minusculas( par.first ) != filtroMin ) continue;
      mostro = true;
      ImprimirCategoria( par.first, par.second );
   }
   if ( !mostro ) {
      std::cout << "\nNo hay productos" << ( filtro.empty() ? "" : " en la categoria '" + filtro + "'" ) << ".\n";
      return filtro.empty() ? 0 : 2;
   }

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