#include <cstdlib>
#include <iostream>
#include <string>
#include "ServicioProductosRemoto.hpp"
#include "Servidor.hpp"
#include "DescubridorServidor.hpp"

// Uso manual:     ./intermediario [puertoPropio] [hostProductos] [puertoProductos]
// Uso descubrimiento (el servidor se busca por broadcast):
//                 ./intermediario [puertoPropio] auto [broadcast] [puertoUDP]
// Ejemplo isla 3:  ./intermediario 8080 auto 172.16.123.63 9090
int main( int argc, char * argv[] ) {
   int puerto = ( argc > 1 ) ? std::atoi( argv[1] ) : 8080;
   std::string hostProductos = ( argc > 2 ) ? argv[2] : "127.0.0.1";
   int puertoProductos = 9091;

   if ( hostProductos == "auto" ) {
      std::string broadcast = ( argc > 3 ) ? argv[3] : "255.255.255.255";
      int puertoUDP = ( argc > 4 ) ? std::atoi( argv[4] ) : 9090;

      ServidorEncontrado encontrado;
      if ( !DescubridorServidor::Buscar( broadcast, puertoUDP, 3, 2, encontrado ) ) {
         std::cerr << "No se encontro ningun servidor de productos en " << broadcast << ":" << puertoUDP << "\n";
         return 1;
      }
      hostProductos = encontrado.ip;
      puertoProductos = encontrado.puerto;
      std::cout << "Servidor de productos descubierto: " << hostProductos << ":" << puertoProductos << "\n";
   } else if ( argc > 3 ) {
      puertoProductos = std::atoi( argv[3] );
   }

   ServicioProductosRemoto servicio( hostProductos, puertoProductos );

   Servidor servidor( puerto );
   if ( !servidor.Iniciar() ) {
      return 1;
   }
   std::cout << "Intermediario -> servidor de productos en " << hostProductos << ":" << puertoProductos << "\n";
   servidor.Ejecutar( servicio );
   return 0;
}