#include "RegistroServidores.hpp"
#include "Texto.hpp"

InfoServidor * RegistroServidores::Buscar( const std::string & host, int puerto ) {
   for ( InfoServidor & s : servidores ) {
      if ( s.host == host && s.puerto == puerto ) return &s;
   }
   return nullptr;
}

bool RegistroServidores::Registrar( const std::string & nombre, const std::string & host, int puerto,const std::vector<std::string> & categorias ) {
   std::lock_guard<std::mutex> lock( mtx );
   bool cambio = false;
   InfoServidor * s = Buscar( host, puerto );
   if ( s == nullptr ) {
      servidores.push_back( InfoServidor() );
      s = &servidores.back();
      s->host = host;
      s->puerto = puerto;
      s->nombre = host + ":" + std::to_string( puerto );
      cambio = true;
   } else if ( !s->activo ) {
      cambio = true;
   }
   if ( !nombre.empty() ) s->nombre = nombre;
   s->categorias = categorias;
   s->activo = true;
   s->ultimoContacto = std::time( nullptr );
   return cambio;
}

bool RegistroServidores::AgregarSemilla( const std::string & host, int puerto ) {
   std::lock_guard<std::mutex> lock( mtx );
   if ( Buscar( host, puerto ) != nullptr ) return false;
   InfoServidor s;
   s.host = host;
   s.puerto = puerto;
   s.nombre = host + ":" + std::to_string( puerto );
   s.activo = false;
   servidores.push_back( s );
   return true;
}

bool RegistroServidores::MarcarInactivo( const std::string & host, int puerto ) {
   std::lock_guard<std::mutex> lock( mtx );
   InfoServidor * s = Buscar( host, puerto );
   if ( s == nullptr || !s->activo ) return false;
   s->activo = false;
   return true;
}

std::string RegistroServidores::Conflicto( const std::string & host, int puerto, const std::vector<std::string> & categorias, const std::string & nombre ) const {
   std::lock_guard<std::mutex> lock( mtx );
   for ( const InfoServidor & s : servidores ) {
      if ( !s.activo || ( s.host == host && s.puerto == puerto ) ) continue;
      if ( !nombre.empty() && s.nombre == nombre && s.puerto == puerto ) continue; // mismo servidor visto con otra IP
      for ( const std::string & nueva : categorias ) {
         for ( const std::string & existente : s.categorias ) {
            if ( IgualesSinMayusculas( nueva, existente ) ) {
               return "la categoria '" + nueva + "' ya la ofrece " + s.nombre;
            }
         }
      }
   }
   return "";
}

std::vector<InfoServidor> RegistroServidores::Todos() const {
   std::lock_guard<std::mutex> lock( mtx );
   return servidores;
}

std::vector<InfoServidor> RegistroServidores::Activos() const {
   std::lock_guard<std::mutex> lock( mtx );
   std::vector<InfoServidor> r;
   for ( const InfoServidor & s : servidores ) {
      if ( s.activo ) r.push_back( s );
   }
   return r;
}

std::vector<InfoServidor> RegistroServidores::ServidoresDeCategoria( const std::string & categoria ) const {
   std::lock_guard<std::mutex> lock( mtx );
   std::vector<InfoServidor> r;
   for ( const InfoServidor & s : servidores ) {
      if ( !s.activo ) continue;
      for ( const std::string & c : s.categorias ) {
         if ( IgualesSinMayusculas( c, categoria ) ) {
            r.push_back( s );
            break;
         }
      }
   }
   return r;
}

std::vector<std::pair<std::string, std::vector<std::string>>> RegistroServidores::Rutas() const {
   std::lock_guard<std::mutex> lock( mtx );
   std::vector<std::pair<std::string, std::vector<std::string>>> rutas;
   for ( const InfoServidor & s : servidores ) {
      if ( !s.activo ) continue;
      for ( const std::string & c : s.categorias ) {
         size_t idx = rutas.size();
         for ( size_t i = 0; i < rutas.size(); i++ ) {
            if ( IgualesSinMayusculas( rutas[i].first, c ) ) { idx = i; break; }
         }
         if ( idx == rutas.size() ) rutas.push_back( { c, {} } );
         rutas[idx].second.push_back( s.nombre );
      }
   }
   return rutas;
}