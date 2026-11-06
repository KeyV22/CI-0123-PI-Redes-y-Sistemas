#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <sstream>
#include <vector>
#include "Descubrimiento.hpp"

static std::vector<std::string> Partir( const std::string & t, char sep ) {
   std::vector<std::string> r;
   std::stringstream ss( t );
   std::string x;
   while ( std::getline( ss, x, sep ) ) r.push_back( x );
   return r;
}

Descubrimiento::Descubrimiento( ClienteRegistro & registro, const std::string & nombre, int puertoHttp,const std::vector<std::string> & broadcasts, int puertoEscucha, int puertoIntermediario ) : registro( registro ), nombre( nombre ), puertoHttp( puertoHttp ), broadcasts( broadcasts ),puertoEscucha( puertoEscucha ), puertoIntermediario( puertoIntermediario ),escucha( 'd', false ), corriendo( false ) {}

Descubrimiento::~Descubrimiento() {
   Detener();
}

void Descubrimiento::Iniciar( int segundos ) {
   corriendo = true;
   try {
      escucha.Bind( puertoEscucha );   // SO_REUSEADDR: varios servidores en una PC pueden compartir el puerto
      escucha.SetReadTimeout( 1 );     // para poder revisar 'corriendo'
      hiloEscucha = std::thread( [this]() { HiloEscucha(); } );
   } catch ( const std::exception & e ) {
      std::cerr << "No se pudo escuchar broadcasts en UDP " << puertoEscucha << ": " << e.what() << "\n";
   }
   hiloBusqueda = std::thread( [this, segundos]() { HiloBusqueda( segundos ); } );
}

void Descubrimiento::Detener() {
   corriendo = false;
   if ( hiloEscucha.joinable() ) hiloEscucha.join();
   if ( hiloBusqueda.joinable() ) hiloBusqueda.join();
}

void Descubrimiento::HiloEscucha() {
   char buf[512];
   while ( corriendo ) {
      std::string ip;
      int puerto = 0;
      long n = escucha.RecvFrom( buf, sizeof( buf ) - 1, ip, puerto );
      if ( n <= 0 ) continue;   // timeout
      std::string msg( buf, n );
      while ( !msg.empty() && ( msg.back() == '\n' || msg.back() == '\r' ) ) msg.pop_back();

      if ( msg == "BUSCAR_PRODUCTOS" ) {
         escucha.SendTo( "PRODUCTOS|" + nombre + "|" + std::to_string( puertoHttp ), ip, puerto );
      }
   }
}

void Descubrimiento::Buscar() {
   try {
      Socket s( 'd', false );          // socket efimero: la respuesta vuelve a su puerto
      s.HabilitarBroadcast();
      s.SetReadTimeout( 2 );
        for ( const std::string & b : broadcasts ) {
            s.SendTo( "BUSCAR_INTERMEDIARIO", b, puertoIntermediario );
        }
      char buf[256];
      std::string ip;
      int puerto = 0;
      long n = s.RecvFrom( buf, sizeof( buf ) - 1, ip, puerto );
      if ( n <= 0 ) return;

      std::vector<std::string> p = Partir( std::string( buf, n ), '|' );
      if ( p.size() >= 2 && p[0] == "INTERMEDIARIO" ) {
         int ph = std::atoi( p[1].c_str() );
         if ( ph > 0 && ph <= 65535 ) registro.FijarIntermediario( ip, ph );
      }
   } catch ( const std::exception & ) {
      // sin red / sin permisos: se reintenta en el siguiente ciclo
   }
}

void Descubrimiento::HiloBusqueda( int segundos ) {
   while ( corriendo ) {
      if ( !registro.Conectado() ) Buscar();
      for ( int i = 0; i < segundos && corriendo; i++ ) {
         std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
      }
   }
}