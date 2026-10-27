#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "Cliente.hpp"
#include "Producto.hpp"
#include "Carrito.hpp"

static std::string LeerLinea( const std::string & prompt ) {
   std::cout << prompt;
   std::string s;
   std::getline( std::cin, s );
   return s;
}

static bool LeerEntero( const std::string & prompt, int & valor ) {
   std::string s = LeerLinea( prompt );
   try {
      size_t usados = 0;
      int v = std::stoi( s, &usados );
      if ( usados != s.size() ) return false;
      valor = v;
      return true;
   } catch ( const std::exception & ) {
      return false;
   }
}

static bool EsSi( const std::string & s ) {
   return s == "s" || s == "S";
}

static bool SiguienteEtiqueta( const std::string & html, const std::string & tag,size_t & pos, std::string & texto ) {
   size_t ini = html.find( "<" + tag, pos );
   if ( ini == std::string::npos ) return false;
   size_t finApertura = html.find( '>', ini );
   size_t cierre = html.find( "</" + tag + ">", finApertura );
   if ( finApertura == std::string::npos || cierre == std::string::npos ) return false;
   texto = html.substr( finApertura + 1, cierre - finApertura - 1 );
   pos = cierre + tag.size() + 3;
   return true;
}

// Convierte la pagina de la proforma en texto legible
static std::string ProformaATexto( const std::string & html ) {
   std::ostringstream out;
   if ( html.find( "<TABLE" ) == std::string::npos ) {
      size_t pos = 0;
      std::string li;
      out << "No se pudo generar la proforma:\n";
      while ( SiguienteEtiqueta( html, "LI", pos, li ) ) {
         out << "  - " << li << "\n";
      }
      return out.str();
   }

   size_t posTabla = html.find( "<TABLE" );
   size_t finTabla = html.find( "</TABLE>", posTabla );
   std::string tabla = html.substr( posTabla, finTabla - posTabla );

   bool primera = true;
   size_t pos = 0;
   std::string fila;
   while ( SiguienteEtiqueta( tabla, "TR", pos, fila ) ) {
      std::vector<std::string> celdas;
      size_t p = 0;
      std::string celda;
      const std::string tag = primera ? "TH" : "TD";
      while ( SiguienteEtiqueta( fila, tag, p, celda ) ) celdas.push_back( celda );
      if ( celdas.size() < 6 ) continue;

      // Bodega, Categoria, Descripcion, Cantidad, Precio, Subtotal
      char linea[256];
      snprintf( linea, sizeof( linea ), "%-10s %-20s %-15s %8s %10s %10s\n",
                celdas[0].c_str(), celdas[1].c_str(), celdas[2].c_str(),
                celdas[3].c_str(), celdas[4].c_str(), celdas[5].c_str() );
      out << linea;
      primera = false;
   }

   size_t posTotal = html.find( "TOTAL:" );
   if ( posTotal != std::string::npos ) {
      size_t finTotal = html.find( '<', posTotal );
      out << html.substr( posTotal, finTotal - posTotal ) << "\n";
   }
   return out.str();
}


static std::vector<std::string> parsearCategorias( const std::string & body ) {
   std::vector<std::string> categorias;
   size_t pos = 0;
   while ( (pos = body.find("<A href=", pos)) != std::string::npos ) {
      size_t inicioTexto = body.find('>', pos);
      if ( inicioTexto == std::string::npos ) break;
      size_t finTexto = body.find("</A>", inicioTexto);
      if ( finTexto == std::string::npos ) break;
      categorias.push_back( body.substr(inicioTexto + 1, finTexto - inicioTexto - 1) );
      pos = finTexto + 4;
   }
   return categorias;
}

// Arma el body application/x-www-form-urlencoded que espera
// POST /TicAmazon/proforma.php: item0_categoria=...&item0_producto=...&item0_cantidad...
static std::string ArmarCuerpoProforma( const std::vector<ItemCarrito> & items ) {
   std::ostringstream cuerpo;
   for ( size_t i = 0; i < items.size(); ++i ) {
      if ( i > 0 ) cuerpo << "&";
      cuerpo << "item" << i << "_categoria=" << UrlEncode(items[i].producto.categoria)
             << "&item" << i << "_producto=" << UrlEncode(items[i].producto.descripcion)
             << "&item" << i << "_cantidad=" << items[i].cantidadElegida;
   }
   return cuerpo.str();
}

static bool ObtenerCategorias( Cliente & cliente, const char * host, const char * servicio,std::vector<std::string> & categorias ) {
   try {
      categorias = parsearCategorias( cliente.Get( host, servicio, "/TicAmazon/list.php" ) );
   } catch ( const std::runtime_error & e ) {
      std::cout << "Error al consultar categorias: " << e.what() << "\n";
      return false;
   }
   if ( categorias.empty() ) {
      std::cout << "El servidor no tiene categorias disponibles.\n";
      return false;
   }
   return true;
}

static void MostrarCategorias( const std::vector<std::string> & categorias ) {
   std::cout << "\nCategorias disponibles:\n";
   for ( size_t i = 0; i < categorias.size(); i++ ) {
      std::cout << "  [" << i << "] " << categorias[i] << "\n";
   }
}

static bool ObtenerProductos( Cliente & cliente, const char * host, const char * servicio,const std::string & categoria, std::vector<Producto> & productos ) {
   try {
      std::string path = "/TicAmazon/list.php?category=" + UrlEncode( categoria );
      productos = parsearProductos( cliente.Get( host, servicio, path ) );
      return true;
   } catch ( const std::exception & e ) {
      std::cout << "Error al consultar productos: " << e.what() << "\n";
      return false;
   }
}

static void MostrarProductos( const std::vector<Producto> & productos ) {
   for ( size_t i = 0; i < productos.size(); i++ ) {
      printf( "[%zu] %s - stock: %d - precio: %.2f (bodega: %s)\n",i, productos[i].descripcion.c_str(), productos[i].cantidad,productos[i].precio, productos[i].bodega.c_str() );
   }
}


static void OpcionListarCategorias( Cliente & cliente, const char * host, const char * servicio ) {
   std::vector<std::string> categorias;
   if ( ObtenerCategorias( cliente, host, servicio, categorias ) ) {
      MostrarCategorias( categorias );
   }
}

static void OpcionVerProductos( Cliente & cliente, const char * host, const char * servicio ) {
   std::vector<std::string> categorias;
   if ( !ObtenerCategorias( cliente, host, servicio, categorias ) ) return;
   MostrarCategorias( categorias );

   int idx;
   if ( !LeerEntero( "Elija el indice de la categoria: ", idx ) || idx < 0 || idx >= (int)categorias.size() ) {
      std::cout << "Indice invalido.\n";
      return;
   }

   std::vector<Producto> productos;
   if ( !ObtenerProductos( cliente, host, servicio, categorias[idx], productos ) ) return;
   if ( productos.empty() ) {
      std::cout << "No se encontraron productos en esa categoria.\n";
   } else {
      std::cout << "\n";
      MostrarProductos( productos );
   }
}

static void OpcionComprar( Cliente & cliente, const char * host, const char * servicio ) {
   Carrito carrito;
   bool seguir = true;

   while ( seguir ) {
      std::vector<std::string> categorias;
      if ( !ObtenerCategorias( cliente, host, servicio, categorias ) ) break;
      MostrarCategorias( categorias );

      int idx;
      if ( !LeerEntero( "Elija el indice de la categoria: ", idx ) || idx < 0 || idx >= (int)categorias.size() ) {
         std::cout << "Indice invalido.\n";
      } else {
         std::vector<Producto> productos;
         if ( ObtenerProductos( cliente, host, servicio, categorias[idx], productos ) ) {
            if ( productos.empty() ) {
               std::cout << "No se encontraron productos en esa categoria.\n";
            } else {
               std::cout << "\n";
               MostrarProductos( productos );
               int ip, cant;
               if ( LeerEntero( "Elija un indice de producto (-1 para ninguno): ", ip )
                    && ip >= 0 && ip < (int)productos.size() ) {
                  if ( !LeerEntero( "Cantidad deseada: ", cant ) || cant <= 0 ) {
                     std::cout << "Cantidad invalida.\n";
                  } else if ( cant > productos[ip].cantidad ) {
                     std::cout << "No hay suficientes productos. Solo hay: " << productos[ip].cantidad << "\n";
                  } else {
                     carrito.Agregar( productos[ip], cant );
                     std::cout << "Producto agregado al carrito.\n";
                  }
               }
            }
         }
      }
      seguir = EsSi( LeerLinea( "Quiere agregar mas productos? (s/n): " ) );
   }

   if ( carrito.Items().empty() ) {
      std::cout << "El carrito esta vacio.\n";
      return;
   }

   carrito.MostrarFactura();

   try {
      std::string respuesta = cliente.Post( host, servicio, "/TicAmazon/proforma.php",ArmarCuerpoProforma( carrito.Items() ) );
      std::cout << "\nFactura confirmada por el servidor\n"<< ProformaATexto( respuesta ) << "\n";
   } catch ( const std::runtime_error & e ) {
      std::cout << "No se pudo confirmar la factura con el servidor: " << e.what() << "\n";
   }
}


int main( int argc, char * argv[] ) {
   const char * host = (argc > 1) ? argv[1] : "os.ecci.ucr.ac.cr";
   const char * servicio = (argc > 2) ? argv[2] : "http";

   Cliente cliente;
   bool salir = false;

   while ( !salir ) {
      std::cout << "\nTicAmazon\n"
                << "1. Listar categorias\n"
                << "2. Ver productos de una categoria\n"
                << "3. Comprar (carrito y factura proforma)\n"
                << "4. Salir\n";
      int opcion;
      bool ok = LeerEntero( "Opcion: ", opcion );
      if ( !std::cin ) break;  
      if ( !ok ) {
         std::cout << "Opcion invalida.\n";
         continue;
      }

      switch ( opcion ) {
         case 1: OpcionListarCategorias( cliente, host, servicio ); break;
         case 2: OpcionVerProductos( cliente, host, servicio ); break;
         case 3: OpcionComprar( cliente, host, servicio ); break;
         case 4: salir = true; break;
         default: std::cout << "Opcion invalida.\n";
      }
   }
   return 0;
}