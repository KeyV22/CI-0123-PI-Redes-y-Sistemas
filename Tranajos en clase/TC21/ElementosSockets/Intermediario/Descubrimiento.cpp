#include <chrono>
#include <cstdlib>
#include <exception>
#include <sstream>
#include <thread>
#include <vector>
#include "Descubrimiento.hpp"
#include "Bitacora.hpp"
#include <vector>

static std::vector<std::string> Partir( const std::string & t, char sep ) {
   std::vector<std::string> r;
   std::stringstream ss( t );
   std::string x;
   while ( std::getline( ss, x, sep ) ) r.push_back( x );
   return r;
}

Descubrimiento::Descubrimiento( ServicioProductosDistribuido & servicio, int puertoHttp,
                                const std::vector<std::string> & broadcasts, int puertoUdp, int puertoProductosUdp )
   : servicio( servicio ), puertoHttp( puertoHttp ), broadcasts( broadcasts ),
     puertoUdp( puertoUdp ), puertoProductosUdp( puertoProductosUdp ), udp( 'd', false ) {}

bool Descubrimiento::Iniciar( int segundos ) {
   try {
      udp.HabilitarBroadcast();
      udp.Bind( puertoUdp );
   } catch ( const std::exception & e ) {
      Bitacora::Registrar( "ERROR", std::string( "No se pudo iniciar el descubrimiento UDP: " ) + e.what(), "intermediario" );
      return false;
   }
   std::thread( [this]() { HiloEscucha(); } ).detach();
   std::thread( [this, segundos]() { HiloBusqueda( segundos ); } ).detach();
    std::string lista;
   for ( const std::string & b : broadcasts ) lista += ( lista.empty() ? "" : ", " ) + b;
    Bitacora::Registrar( "INFO", "Descubrimiento UDP activo: escucha en " + std::to_string( puertoUdp )+ ", broadcast a " + lista, "intermediario" );   return true;
}

void Descubrimiento::HiloBusqueda( int segundos ) {
   while ( true ) {
      for ( const std::string & b : broadcasts ) {
         udp.SendTo( "BUSCAR_PRODUCTOS", b, puertoProductosUdp );
      }
      std::this_thread::sleep_for( std::chrono::seconds( segundos ) );
   }
}

void Descubrimiento::HiloEscucha() {
   char buf[512];
   while ( true ) {
      std::string ip;
      int puerto = 0;
      long n = udp.RecvFrom( buf, sizeof( buf ) - 1, ip, puerto );
      if ( n < 0 ) {
         std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
         continue;
      }
      std::string msg( buf, n );
      while ( !msg.empty() && ( msg.back() == '\n' || msg.back() == '\r' ) ) msg.pop_back();

      std::vector<std::string> p = Partir( msg, '|' );
      if ( p.empty() ) continue;

      if ( p[0] == "BUSCAR_INTERMEDIARIO" ) {
         udp.SendTo( "INTERMEDIARIO|" + std::to_string( puertoHttp ), ip, puerto );
      } else if ( p[0] == "PRODUCTOS" && p.size() >= 3 ) {
         int ph = std::atoi( p[2].c_str() );
         if ( ph > 0 && ph <= 65535 ) {
            servicio.DescubrirServidor( ip, ph, p[1] );
         }
      }
   }
}