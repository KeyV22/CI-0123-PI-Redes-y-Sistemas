#include <iostream>
#include <cassert>
#include "Protocolo.hpp"

int main() {

   // construir + parsear
   std::string m = construirMensaje( "CLI_04", "INT_04", TipoMensaje::ADD_TO_CART, { "CafeFrio", "2" } );
   std::cout << "Construido: " << m << "\n";
   assert( m == "CLI_04|INT_04/30/CafeFrio/2" );

   MensajeV2 p = parsearMensaje( m );
   assert( p.origen == "CLI_04" );
   assert( p.destino == "INT_04" );
   assert( p.tipo == 30 );
   assert( p.campos.size() == 2 );
   assert( p.campos[0] == "CafeFrio" );
   assert( p.campos[1] == "2" );
   std::cout << "Parseo OK: origen=" << p.origen << " destino=" << p.destino << " tipo=" << p.tipo << "\n";

   // validaciones
   assert( ValidarOrigenDestino( "CLI_04" ) );
   assert( ValidarOrigenDestino( "INT_02" ) );
   assert( ValidarOrigenDestino( "SERV" ) );
   assert( !ValidarOrigenDestino( "CLIENTE_04" ) );

   assert( ValidarCategoria( "Alimentos" ) );
   assert( !ValidarCategoria( "AB" ) );          // menos de 3 caracteres
   assert( !ValidarCategoria( "Alimentos123" ) ); // no permite digitos

   assert( ValidarPrecio( "650.50" ) );
   assert( ValidarPrecio( "3" ) );
   assert( !ValidarPrecio( "abc" ) );

   assert( ValidarStock( "40" ) );
   assert( !ValidarStock( "-5" ) );

   std::cout << "Validaciones regex: OK\n";

   // error 90 (formato)
   ResultadoValidacion r;
   bool ok = ValidarCampoPorTipo( "categoria", "AB", &r );
   assert( !ok );
   std::string err90 = construirError90( "INT_04", "CLI_04", TipoMensaje::REQUEST_PRODUCTS_C, r.campoInvalido, r.valorRecibido );
   std::cout << "Error 90 generado: " << err90 << "\n";

   // error 91 (tamano) -- categoria de 25 caracteres excede el maximo de 20
   ResultadoValidacion r2;
   bool ok2 = ValidarCampoPorTipo( "categoria", "unacategoriaconnombredemasiadolargo", &r2 );
   assert( !ok2 );
   assert( r2.esErrorTamano );
   std::string err91 = construirError91( "INT_04", "CLI_04", TipoMensaje::REQUEST_PRODUCTS_C, "categoria", r2.tamanoRecibido, r2.tamanoMaximo );
   std::cout << "Error 91 generado: " << err91 << "\n";

   // error 92 (sin respuesta)
   std::string err92 = construirError92( "INT_04", "CLI_04", "SERV", TipoMensaje::REQUEST_PRODUCTS_B, 2 );
   std::cout << "Error 92 generado: " << err92 << "\n";

   std::cout << "\nTODAS LAS PRUEBAS PASARON\n";

   return 0;

}
