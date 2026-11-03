#include <chrono>
#include <exception>
#include <sstream>
#include "ClienteRegistro.hpp"
#include "Socket.hpp"
#include "PaginasHtml.hpp"
#include "Bitacora.hpp"

ClienteRegistro::ClienteRegistro( Inventario & inventario, const std::string & nombre, int miPuerto ): inventario( inventario ), nombre( nombre ), miPuerto( miPuerto ),puertoIntermediario( 0 ), conectado( false ), corriendo( false ) {}

ClienteRegistro::~ClienteRegistro() {
   Detener();
}

void ClienteRegistro::FijarIntermediario( const std::string & host, int puerto ) {
   bool cambio;
   {
      std::lock_guard<std::mutex> lock( mtx );
      cambio = ( host != hostIntermediario || puerto != puertoIntermediario );
      hostIntermediario = host;
      puertoIntermediario = puerto;
   }
   if ( cambio ) {
      Bitacora::Registrar( "Intermediario encontrado en " + host + ":" + std::to_string( puerto ) );
   }
}

bool ClienteRegistro::EnviarRegistro() {
   std::string host;
   int puerto;
   {
      std::lock_guard<std::mutex> lock( mtx );
      host = hostIntermediario;
      puerto = puertoIntermediario;
   }
   if ( host.empty() || puerto <= 0 ) return false;   // aun no se descubre

   try {
      Socket s( 's', false );
      s.SetReadTimeout( 3 );
      s.Connect( host.c_str(), puerto );

      // La IP con la que el intermediario nos ve es la que debe usar para llamarnos
      std::string miHost = s.IpLocal();

      std::string cats;
      for ( const std::string & c : inventario.ListarCategorias() ) {
         if ( !cats.empty() ) cats += ";";
         cats += c;
      }
      std::string cuerpo = "nombre=" + PaginasHtml::CodificarUrl( nombre ) + "&host=" + PaginasHtml::CodificarUrl( miHost )+ "&puerto=" + std::to_string( miPuerto ) + "&categorias=" + PaginasHtml::CodificarUrl( cats );

      std::ostringstream peticion;
      peticion << "POST /TicAmazon/registrar HTTP/1.1\r\n"<< "Host: " << host << "\r\n"<< "Connection: close\r\n"<< "Content-Type: application/x-www-form-urlencoded\r\n"<< "Content-Length: " << cuerpo.size() << "\r\n"<< "\r\n" << cuerpo;
      std::string request = peticion.str();
      s.Write( request.data(), request.size() );

      std::string resp;
      char buf[1024];
      while ( resp.size() < 8192 ) {
         size_t n = s.Read( buf, sizeof( buf ) );
         if ( n == 0 ) break;
         resp.append( buf, n );
      }
      return resp.compare( 0, 12, "HTTP/1.1 200" ) == 0;
   } catch ( const std::exception & ) {
      return false;
   }
}

void ClienteRegistro::Despedirse() {
   std::string host;
   int puerto;
   {
      std::lock_guard<std::mutex> lock( mtx );
      host = hostIntermediario;
      puerto = puertoIntermediario;
   }
   if ( host.empty() || puerto <= 0 || !conectado ) return;   // nunca estuvo registrado

   try {
      Socket s( 's', false );
      s.SetReadTimeout( 3 );
      s.Connect( host.c_str(), puerto );
      // Mismo host y puerto con los que se registro
      std::string cuerpo = "host=" + PaginasHtml::CodificarUrl( s.IpLocal() ) + "&puerto=" + std::to_string( miPuerto );

      std::ostringstream peticion;
      peticion << "POST /TicAmazon/desregistrar HTTP/1.1\r\n"<< "Host: " << host << "\r\n"<< "Connection: close\r\n"<< "Content-Type: application/x-www-form-urlencoded\r\n"<< "Content-Length: " << cuerpo.size() << "\r\n"<< "\r\n" << cuerpo;
      std::string request = peticion.str();
      s.Write( request.data(), request.size() );

      char buf[512];
      s.Read( buf, sizeof( buf ) );   // espera la respuesta para no cortar el aviso
      Bitacora::Registrar( "Se aviso al intermediario que este servidor se apaga" );
   } catch ( const std::exception & ) {
      // el intermediario no esta: no hay a quien avisar
   }
}
void ClienteRegistro::Iniciar( int segundos ) {
   corriendo = true;
   hilo = std::thread( [this, segundos]() {
      bool registrado = false;
      while ( corriendo ) {
         bool ok = EnviarRegistro();
         if ( ok && !registrado ) {
            Bitacora::Registrar( "Registrado en el intermediario como '" + nombre + "'" );
         } else if ( !ok && registrado ) {
            Bitacora::Registrar( "Se perdio contacto con el intermediario, se reintentara" );
         }
         registrado = ok;
         conectado = ok;

         for ( int i = 0; i < segundos && corriendo; i++ ) {
            std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
         }
      }
   } );
}

void ClienteRegistro::Detener() {
   corriendo = false;
   if ( hilo.joinable() ) hilo.join();
}