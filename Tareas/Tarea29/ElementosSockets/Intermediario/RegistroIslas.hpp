#ifndef RegistroIslas_hpp
#define RegistroIslas_hpp

#include <ctime>
#include <mutex>
#include <string>
#include <vector>

// Un intermediario de OTRA isla, descubierto por ANNOUNCE (60).
struct InfoIsla {
   std::string id;                        // INT_03
   std::string host;                      // ip que anuncio
   int puerto = 0;                        // puerto TCP del protocolo mancomunado
   std::vector<std::string> categorias;   // ultimas categorias conocidas (GET_CATEGORIES)
   bool activa = false;
   std::time_t ultimoContacto = 0;
};

// Tabla de rutas hacia las otras islas.
class RegistroIslas {
   public:
      // Crea o actualiza la isla y la marca activa. true si era nueva o estaba inactiva.
      bool Registrar( const std::string & id, const std::string & host, int puerto );
      void ActualizarCategorias( const std::string & id, const std::vector<std::string> & categorias );
      // true si estaba activa y paso a inactiva
      bool MarcarInactiva( const std::string & id );

      bool Obtener( const std::string & id, InfoIsla & resultado ) const;
      std::vector<InfoIsla> Todas() const;
      // Islas activas que declaran la categoria (formato del protocolo: minusculas y '_')
      std::vector<InfoIsla> IslasDeCategoria( const std::string & categoriaProtocolo ) const;

   private:
      mutable std::mutex mtx;
      std::vector<InfoIsla> islas;
};

#endif