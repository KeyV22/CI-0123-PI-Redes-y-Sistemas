#ifndef RegistroServidores_hpp
#define RegistroServidores_hpp

#include <ctime>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

struct InfoServidor {
   std::string nombre;
   std::string host;
   int puerto = 0;
   std::vector<std::string> categorias;
   bool activo = false;
   std::time_t ultimoContacto = 0;
};

// Tabla de servidores de productos
class RegistroServidores {
   public:
      // Crea o actualiza (clave: host+puerto) y lo marca activo
      // nombre vacio = conservar el que ya tenia. Devuelve true si era nuevo
      // o estaba inactivo
      bool Registrar( const std::string & nombre, const std::string & host, int puerto,const std::vector<std::string> & categorias );

      // Servidor conocido de antemano entra como inactivo hasta que responda
      bool AgregarSemilla( const std::string & host, int puerto );

      // Devuelve true si estaba activo y paso a inactivo
      bool MarcarInactivo( const std::string & host, int puerto );

      std::vector<InfoServidor> Todos() const;
      std::vector<InfoServidor> Activos() const;
      std::vector<InfoServidor> ServidoresDeCategoria( const std::string & categoria ) const;
      std::vector<std::pair<std::string, std::vector<std::string>>> Rutas() const;

   private:
      InfoServidor * Buscar( const std::string & host, int puerto );   

      mutable std::mutex mtx;
      std::vector<InfoServidor> servidores;
};

#endif