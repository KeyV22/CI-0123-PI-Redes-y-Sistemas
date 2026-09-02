/**
 *  Cliente TicAmazon - version simple (borrador)
 *
 *  Un solo archivo, estilo directo, sin separar en varias clases.
 *  Hace un GET por HTTP usando la jerarquia Socket/VSocket, muestra
 *  los productos de una categoria y arma una factura sencilla.
 *
 *  Compilar:  make
 *  Ejecutar:  ./cliente
 **/

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cctype>
#include "Socket.h"

using namespace std;

// ---------- codificar espacios y acentos para que quepan en la URL ----------
string codificarUrl( const string & texto ) {
   ostringstream salida;
   for ( unsigned char c : texto ) {
      if ( isalnum(c) ) {
         salida << c;
      } else {
         salida << '%' << uppercase << hex << setw(2) << setfill('0') << (int)c;
      }
   }
   return salida.str();
}

// ---------- pedir la pagina al servidor y devolver solo el body ----------
string pedirPagina( const string & host, const string & path ) {
   Socket s( 's', false );            // socket TCP, ipv4
   s.Connect( host.c_str(), "http" );

   string peticion = "GET " + path + " HTTP/1.1\r\nhost: " + host + "\r\nConnection: close\r\n\r\n";
   s.Write( peticion.c_str() );

   // leemos todo hasta que el servidor cierre la conexion
   string respuesta;
   char buffer[512];
   size_t leidos;
   while ( ( leidos = s.Read( buffer, sizeof(buffer) ) ) > 0 ) {
      respuesta.append( buffer, leidos );
   }

   size_t finHeaders = respuesta.find("\r\n\r\n");
   if ( finHeaders == string::npos ) {
      return "";                      // no llego una respuesta valida
   }
   return respuesta.substr( finHeaders + 4 );
}

// ---------- sacar el texto que hay entre <TD> y </TD>, empezando en pos ----------
string siguienteCelda( const string & html, size_t & pos ) {
   size_t inicio = html.find( ">", html.find( "<TD", pos ) );
   size_t fin    = html.find( "</TD>", inicio );
   pos = fin + 5;
   return html.substr( inicio + 1, fin - inicio - 1 );
}

// ---------- estructura simple para un producto ----------
struct Producto {
   string descripcion;
   string bodega;
   int    cantidad;
   double precio;
};

// ---------- recorrer las filas <TR> de la tabla y armar la lista ----------
vector<Producto> listarProductos( const string & body ) {
   vector<Producto> lista;
   size_t pos = 0;

   while ( ( pos = body.find( "<TR>", pos ) ) != string::npos ) {
      size_t p = pos;
      siguienteCelda( body, p );                 // intermediario (no lo usamos)
      string bodega      = siguienteCelda( body, p );
      siguienteCelda( body, p );                 // categoria (no lo usamos)
      string descripcion = siguienteCelda( body, p );
      string cantidadTxt = siguienteCelda( body, p );
      string precioTxt   = siguienteCelda( body, p );

      Producto prod;
      prod.descripcion = descripcion;
      prod.bodega      = bodega;
      prod.cantidad    = atoi( cantidadTxt.c_str() );
      prod.precio      = atof( precioTxt.c_str() );
      lista.push_back( prod );

      pos = p;
   }
   return lista;
}

int main() {
   const string host = "os.ecci.ucr.ac.cr";

   vector<Producto> carrito;
   vector<int>      cantidades;

   char seguir = 's';
   while ( seguir == 's' || seguir == 'S' ) {
      cout << "Ingrese una categoria (ejemplo: Alimentos y bebidas): ";
      cin.ignore();
      string categoria;
      getline( cin, categoria );

      string path = "/TicAmazon/list.php?category=" + codificarUrl( categoria );
      string body = pedirPagina( host, path );

      vector<Producto> productos = listarProductos( body );

      if ( productos.empty() ) {
         cout << "No se encontraron productos en esa categoria.\n";
      } else {
         for ( size_t i = 0; i < productos.size(); i++ ) {
            cout << "[" << i << "] " << productos[i].descripcion
                 << " - stock: " << productos[i].cantidad
                 << " - precio: " << productos[i].precio
                 << " (bodega: " << productos[i].bodega << ")\n";
         }

         cout << "Elija un indice de producto (-1 para ninguno): ";
         int indice;
         cin >> indice;

         if ( indice >= 0 && indice < (int)productos.size() ) {
            cout << "Cantidad deseada: ";
            int cantidad;
            cin >> cantidad;

            if ( cantidad > productos[indice].cantidad ) {
               cout << "No hay suficientes productos. Solo hay: "
                    << productos[indice].cantidad << "\n";
            } else {
               carrito.push_back( productos[indice] );
               cantidades.push_back( cantidad );
               cout << "Producto agregado al carrito.\n";
            }
         }
      }

      cout << "Quiere buscar otra categoria? (s/n): ";
      cin >> seguir;
   }

   // ---------- factura ----------
   double total = 0.0;
   cout << "\nFACTURA\n";
   for ( size_t i = 0; i < carrito.size(); i++ ) {
      double subtotal = carrito[i].precio * cantidades[i];
      total += subtotal;
      cout << carrito[i].descripcion << "  x" << cantidades[i]
           << "  precio: " << carrito[i].precio
           << "  subtotal: " << subtotal << "\n";
   }
   cout << "TOTAL: " << total << "\n";

   return 0;
}
