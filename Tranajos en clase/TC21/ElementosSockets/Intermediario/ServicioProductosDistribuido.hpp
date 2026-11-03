#ifndef ServicioProductosDistribuido_hpp
#define ServicioProductosDistribuido_hpp

#include <string>
#include <vector>
#include "ServicioProductos.hpp"
#include "RegistroServidores.hpp"
#include "PaginasHtml.hpp"
#include "RegistroIslas.hpp"

class Mancomunado;
class ServicioProductosDistribuido : public ServicioProductos {
   public:
      void AgregarSemilla( const std::string & host, int puerto );
      void IniciarMonitor( int segundos );     // hilo que sondea a todos cada N segundos
      // Conecta el servicio con la comunicacion entre islas
      void FijarMancomunado( Mancomunado * m, RegistroIslas * islas );

      // Solo lo que hay en los servidores de productos de ESTA isla (lo usa el protocolo mancomunado)
      std::vector<std::string> CategoriasLocales() const;
      void ProductosLocales( const std::string & categoria, std::vector<FilaProducto> & filas ) const;
      void DescubrirServidor( const std::string & host, int puerto, const std::string & nombre );

      RespuestaServicio ListarCategorias() const override;
      RespuestaServicio ListarProductos( const std::string & categoria ) const override;
      RespuestaServicio GenerarProforma( const std::vector<SolicitudItem> & items ) const override;

      RespuestaServicio ListarServidores() const override;
      RespuestaServicio ListarRutas() const override;
      RespuestaServicio RegistrarServidor( const std::string & nombre, const std::string & host, int puerto,const std::vector<std::string> & categorias ) const override;
      RespuestaServicio DesregistrarServidor( const std::string & host, int puerto ) const override;
      RespuestaServicio ListarBitacora( const std::string & filtroTipo ) const override;
   private:
      void Sondear() const;
      bool SondearServidor( const InfoServidor & s ) const;
      bool ConsultarProductos( const InfoServidor & s, const std::string & categoria,std::vector<FilaProducto> & filas ) const;
      
      Mancomunado * mancomunado = nullptr;
      RegistroIslas * islasRemotas = nullptr;

      mutable RegistroServidores registro;     // tiene su propio mutex
};

#endif