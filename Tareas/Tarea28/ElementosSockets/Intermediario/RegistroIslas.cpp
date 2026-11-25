#include "RegistroIslas.hpp"
#include "Mancomunado.hpp"

bool RegistroIslas::Registrar( const std::string & id, const std::string & host, int puerto ) {
   std::lock_guard<std::mutex> lock( mtx );
   for ( InfoIsla & i : islas ) {
      if ( i.id != id ) continue;
      bool cambio = !i.activa || i.host != host || i.puerto != puerto;
      i.host = host;
      i.puerto = puerto;
      i.activa = true;
      i.ultimoContacto = std::time( nullptr );
      return cambio;
   }
   InfoIsla nueva;
   nueva.id = id;
   nueva.host = host;
   nueva.puerto = puerto;
   nueva.activa = true;
   nueva.ultimoContacto = std::time( nullptr );
   islas.push_back( nueva );
   return true;
}

void RegistroIslas::ActualizarCategorias( const std::string & id, const std::vector<std::string> & categorias ) {
   std::lock_guard<std::mutex> lock( mtx );
   for ( InfoIsla & i : islas ) {
      if ( i.id == id ) {
         i.categorias = categorias;
         i.ultimoContacto = std::time( nullptr );
         return;
      }
   }
}

bool RegistroIslas::MarcarInactiva( const std::string & id ) {
   std::lock_guard<std::mutex> lock( mtx );
   for ( InfoIsla & i : islas ) {
      if ( i.id == id && i.activa ) {
         i.activa = false;
         return true;
      }
   }
   return false;
}

bool RegistroIslas::Obtener( const std::string & id, InfoIsla & resultado ) const {
   std::lock_guard<std::mutex> lock( mtx );
   for ( const InfoIsla & i : islas ) {
      if ( i.id == id ) {
         resultado = i;
         return true;
      }
   }
   return false;
}

std::vector<InfoIsla> RegistroIslas::Todas() const {
   std::lock_guard<std::mutex> lock( mtx );
   return islas;
}

std::vector<InfoIsla> RegistroIslas::IslasDeCategoria( const std::string & categoriaProtocolo ) const {
   std::lock_guard<std::mutex> lock( mtx );
   std::vector<InfoIsla> r;
   for ( const InfoIsla & i : islas ) {
      if ( !i.activa ) continue;
      for ( const std::string & c : i.categorias ) {
         if ( Mancomunado::NombreProtocolo( c ) == categoriaProtocolo ) {
            r.push_back( i );
            break;
         }
      }
   }
   return r;
}