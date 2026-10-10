#include <cstdlib>
#include <iostream>
#include <string>
#include "ServicioProductosRemoto.hpp"
#include "Servidor.hpp"

// Uso: ./intermediario [puertoPropio] [hostProductos] [puertoProductos]
int main( int argc, char * argv[] ) {
   int puerto = ( argc > 1 ) ? std::atoi( argv[1] ) : 8080;
   std::string hostProductos = ( argc > 2 ) ? argv[2] : "127.0.0.1";
   int puertoProductos = ( argc > 3 ) ? std::atoi( argv[3] ) : 9090;

   ServicioProductosRemoto servicio( hostProductos, puertoProductos );

   Servidor servidor( puerto );
   if ( !servidor.Iniciar() ) {
      return 1;
   }
   std::cout << "Intermediario -> servidor de productos en " << hostProductos << ":" << puertoProductos << "\n";
   servidor.Ejecutar( servicio );
   return 0;
}