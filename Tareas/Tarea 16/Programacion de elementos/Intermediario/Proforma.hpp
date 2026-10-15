#ifndef Proforma_hpp
#define Proforma_hpp

#include <string>
#include <vector>

// Lo que pide el cliente, que producto y cuantas unidades
struct SolicitudItem {
   std::string categoria;
   std::string descripcion;
   int cantidad;
};

struct LineaProforma {
   std::string bodega;
   std::string categoria;
   std::string descripcion;
   int cantidad;
   double precioUnitario;

   double Subtotal() const { return cantidad * precioUnitario; }
};

class Proforma {
   public:
      Proforma() : total( 0.0 ) {}

      void AgregarLinea( const LineaProforma & l ) {
         lineas.push_back( l );
         total += l.Subtotal();
      }
      void AgregarError( const std::string & e ) { errores.push_back( e ); }

      bool EsValida() const { return errores.empty(); }
      double Total() const { return total; }
      const std::vector<LineaProforma> & Lineas() const { return lineas; }
      const std::vector<std::string> & Errores() const { return errores; }

   private:
      std::vector<LineaProforma> lineas;
      std::vector<std::string> errores;
      double total;
};

#endif