#include <iostream>
#include <string>
#include <vector>
#include <poll.h>
#include <unistd.h>

#include "Cliente.hpp"
#include "Producto.hpp"
#include "Carrito.hpp"
#include "Secundaria.hpp"

static const int INTERVALO_BITACORA = 2000;

static bool EsSi( const std::string & s ) {
   return s == "s" || s == "S";
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

static void OpcionServidores( Cliente & cliente, const char * host, const char * servicio ) {
   ConsultarMostrar( cliente, host, servicio, RUTA_SERVIDORES, "Servidores de productos registrados" );
}
 
static void OpcionRutas( Cliente & cliente, const char * host, const char * servicio ) {
   ConsultarMostrar( cliente, host, servicio, RUTA_RUTAS, "Tabla de rutas (categorias por servidor)" );
}
 
// Bitacora en "tiempo real": consulta cada INTERVALO_BITACORA_MS y solo
// imprime las lineas nuevas. Se detiene al presionar Enter.
static void OpcionBitacora( Cliente & cliente, const char * host, const char * servicio ) {
   std::cout << "\nBitacora de eventos (presione Enter para volver al menu)\n";
   long ultimoID = 0;
   bool seguir = true;
 
   while ( seguir ) {
      try {
         std::string cuerpo = cliente.Get( host, servicio, RUTA_BITACORA );
         std::vector<std::string> nuevos;;
         ExtraerEventosNuevos( cuerpo, ultimoID, nuevos );
         for ( const std::string & linea : nuevos ) {
            std::cout << linea << "\n";
         }
      } catch ( const std::exception & e ) {
         MostrarError( "Error al consultar la bitacora", e );
         break;
      }
 
      struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
      if ( poll( &pfd, 1, INTERVALO_BITACORA ) > 0 ) {
         std::string resto;
         std::getline( std::cin, resto );
         seguir = false;
      }
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
                << "4. Servidores de productos registrados\n"
                << "5. Tabla de rutas \n"
                << "6. Bitácora de eventos\n"
                << "7. Salir\n";
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
         case 4: OpcionServidores( cliente, host, servicio ); break;
         case 5: OpcionRutas( cliente, host, servicio ); break;
         case 6: OpcionBitacora( cliente, host, servicio ); break;
         case 7: salir = true; break;
         default: std::cout << "Opcion invalida.\n";
      }
   }
   return 0;
}