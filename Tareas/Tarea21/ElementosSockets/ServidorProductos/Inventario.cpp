#include <cstdio>
#include <exception>
#include <iostream>
#include "Inventario.hpp"
#include "Texto.hpp"
#include <mutex>

Inventario::Inventario( Almacenamiento & almacen ) : almacen( almacen ) {}

std::vector<Producto> Inventario::LeerTodoSinBloqueo() const {
   std::vector<Producto> resultado;

   for ( const std::string & bodega : almacen.listar_bodegas() ) {
      for ( const ProductoTexto & t : almacen.listar_productos( bodega ) ) {
         try {
            int cantidad = std::stoi( t.cantidad );
            double precio = std::stod( t.precio );
            resultado.push_back( Producto( bodega, t.categoria, t.producto, cantidad, precio ) );
         } catch ( const std::exception & ) {
            std::cerr << "Registro invalido: bodega " << bodega << ", producto " << t.producto << "\n";
         }
      }
   }
   return resultado;
}

std::vector<std::string> Inventario::ListarCategorias() const {
   std::vector<Producto> todos;
   {
      std::lock_guard<std::mutex> lock( mtx );
      todos = LeerTodoSinBloqueo();
   }
   std::vector<std::string> categorias;
   for ( const Producto & p : todos ) {
      bool existe = false;
      for ( const std::string & c : categorias ) {
         if ( IgualesSinMayusculas( c, p.Categoria() ) ) {
            existe = true;
            break;
         }
      }
      if ( !existe ) categorias.push_back( p.Categoria() );
   }
   return categorias;
}

std::vector<Producto> Inventario::ListarProductos( const std::string & categoria ) const {
   std::vector<Producto> todos;
   {
      std::lock_guard<std::mutex> lock( mtx );
      todos = LeerTodoSinBloqueo();
   }

   std::vector<Producto> resultado;
   for ( const Producto & p : todos ) {
      if ( IgualesSinMayusculas( p.Categoria(), categoria ) ) {
         resultado.push_back( p );
      }
   }
   return resultado;
}

Proforma Inventario::GenerarProforma( const std::vector<SolicitudItem> & items ) const {
   std::lock_guard<std::mutex> lock( mtx );
   std::vector<Producto> todos = LeerTodoSinBloqueo();

   Proforma pf;

   for ( const SolicitudItem & it : items ) {
      if ( it.cantidad <= 0 ) {
         pf.AgregarError( "Cantidad invalida para '" + it.descripcion + "'" );
         continue;
      }

      const Producto * prod = nullptr;
      bool categoriaExiste = false;
      for ( const Producto & p : todos ) {
         if ( !IgualesSinMayusculas( p.Categoria(), it.categoria ) ) continue;
         categoriaExiste = true;
         if ( IgualesSinMayusculas( p.Descripcion(), it.descripcion ) ) {
            prod = &p;
            break;
         }
      }

      if ( !categoriaExiste ) {
         pf.AgregarError( "La categoria '" + it.categoria + "' no existe" );
         continue;
      }
      if ( prod == nullptr ) {
         pf.AgregarError( "El producto '" + it.descripcion + "' no existe en la categoria '"+ it.categoria + "'" );
         continue;
      }

      // Si el mismo producto viene en varias lineas, se suman para validar el stock
      int yaPedido = 0;
      for ( const LineaProforma & l : pf.Lineas() ) {
         if ( l.bodega == prod->Bodega()
              && IgualesSinMayusculas( l.categoria, prod->Categoria() )
              && IgualesSinMayusculas( l.descripcion, prod->Descripcion() ) ) {
            yaPedido += l.cantidad;
         }
      }

      if ( yaPedido + it.cantidad > prod->Cantidad() ) {
         pf.AgregarError( "Stock insuficiente para '" + it.descripcion + "': solicitado "+ std::to_string( yaPedido + it.cantidad ) + ", disponible "+ std::to_string( prod->Cantidad() ) );
         continue;
      }

      LineaProforma linea;
      linea.bodega = prod->Bodega();
      linea.categoria = prod->Categoria();
      linea.descripcion = prod->Descripcion();
      linea.cantidad = it.cantidad;
      linea.precioUnitario = prod->Precio();
      pf.AgregarLinea( linea );
   }
   // Compra valida: descontar del almacenamiento lo que se vendio
   if ( pf.EsValida() ) {
      for ( const LineaProforma & l : pf.Lineas() ) {
         int actual = -1;
         for ( const ProductoTexto & t : almacen.listar_productos( l.bodega ) ) {
            if ( t.producto == l.descripcion ) { actual = std::stoi( t.cantidad ); break; }
         }
         if ( actual < 0 ) continue;
         char precio[64];
         snprintf( precio, sizeof( precio ), "%.2f", l.precioUnitario );
         almacen.extraer_producto( l.bodega, l.descripcion );
         almacen.insertar_producto( l.bodega, l.categoria, l.descripcion,
                                    std::to_string( actual - l.cantidad ), precio );
      }
   }
   return pf;
}