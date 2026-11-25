#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <random>
#include <regex>
#include <sstream>
#include <thread>
#include "Mancomunado.hpp"
#include "ServicioProductosDistribuido.hpp"
#include "Bitacora.hpp"
#include "Redes.hpp"

// Expresiones regulares del protocolo (seccion "Expresiones regulares para validacion")
static const std::regex RE_ID_MSG( "^[0-9]{4}$" );
static const std::regex RE_ID_ISLA( "^INT_[0-9]{2}$" );
static const std::regex RE_IP( "^([0-9]{1,3}\\.){3}[0-9]{1,3}$" );
static const std::regex RE_PUERTO( "^[0-9]{1,5}$" );
static const std::regex RE_NOMBRE( "^[a-zA-Z0-9_\\-]{1,50}$" );

static std::vector<std::string> Partir( const std::string & t, char sep ) {
   std::vector<std::string> r;
   std::stringstream ss( t );
   std::string x;
   while ( std::getline( ss, x, sep ) ) r.push_back( x );
   if ( !t.empty() && t.back() == sep ) r.push_back( "" );
   return r;
}

static std::string Minusculas( std::string s ) {
   for ( char & c : s ) c = (char)std::tolower( (unsigned char)c );
   return s;
}

static void QuitarFinLinea( std::string & s ) {
   while ( !s.empty() && ( s.back() == '\n' || s.back() == '\r' ) ) s.pop_back();
}

std::string Mancomunado::NombreProtocolo( const std::string & texto ) {
   std::string r;
   for ( char c : texto ) {
      if ( c == ' ' ) r += '_';
      else if ( std::isalnum( (unsigned char)c ) || c == '_' || c == '-' ) r += (char)std::tolower( (unsigned char)c );
   }
   return r;
}

std::string Mancomunado::IdIsla( int numero ) {
   char buf[16];
   std::snprintf( buf, sizeof( buf ), "INT_%02d", numero );
   return buf;
}

bool Mancomunado::DetectarIsla( std::string & ip, int & numeroIsla ) {
   struct ifaddrs * lista = nullptr;
   if ( getifaddrs( &lista ) != 0 ) return false;
   bool hallada = false;
   for ( struct ifaddrs * a = lista; a != nullptr && !hallada; a = a->ifa_next ) {
      if ( !a->ifa_addr || a->ifa_addr->sa_family != AF_INET ) continue;
      char buf[INET_ADDRSTRLEN];
      inet_ntop( AF_INET, &( (struct sockaddr_in *)a->ifa_addr )->sin_addr, buf, sizeof( buf ) );
      std::string s = buf;
      if ( s.compare( 0, 11, "172.16.123." ) != 0 ) continue;
      int ultimo = std::atoi( s.substr( 11 ).c_str() );
      ip = s;
      numeroIsla = ultimo / 16;       // .16-.31 -> 1, .32-.47 -> 2, ...
      hallada = true;
   }
   freeifaddrs( lista );
   return hallada;
}

std::vector<std::pair<std::string, std::string>> Mancomunado::DestinosPorDefecto( int propia ) {
   std::vector<std::pair<std::string, std::string>> r;
   for ( int n = 1; n <= 6; n++ ) {
      if ( n == propia ) continue;
      r.push_back( { IdIsla( n ), "172.16.123." + std::to_string( n * 16 + 15 ) } );   // .31 .47 .63 .79 .95 .111
   }
   return r;
}

Mancomunado::Mancomunado( ServicioProductosDistribuido & servicio, RegistroIslas & islas, const ConfigMancomunado & config )
   : servicio( servicio ), islas( islas ), config( config ), miId( IdIsla( config.numeroIsla ) ),
     udp( 'd', false ), escuchaTcp( 's', false ), activo( false ), contador( 0 ) {
   // El ID_MSG arranca en un valor al azar: si el intermediario se reinicia no repite los ids de antes
   std::random_device rd;
   contador = (int)( rd() % 10000 );
}

bool Mancomunado::Iniciar() {
   try {
      udp.HabilitarBroadcast();
      udp.Bind( config.puertoUdp );
      udp.SetReadTimeout( 1 );
      
      // Con IP de la red privada, el TCP escucha solo en esa interfaz
      if ( EmpiezaCon( config.ipPrivada, PREFIJO_RED_PRIVADA ) ) escuchaTcp.Bind( config.ipPrivada.c_str(), config.puertoTcp );
      else escuchaTcp.Bind( config.puertoTcp );
      escuchaTcp.Listen( 16 );
   } catch ( const std::exception & e ) {
      Bitacora::Registrar( "ERROR", std::string( "Mancomunado: no se pudo iniciar: " ) + e.what(), "mancomunado" );
      return false;
   }
   activo = true;
   std::thread( [this]() { HiloUdp(); } ).detach();
   std::thread( [this]() { HiloTcp(); } ).detach();
   // Un hilo por cada una de las otras islas
   for ( const auto & d : config.destinos ) {
      std::string id = d.first, bc = d.second;
      std::thread( [this, id, bc]() { HiloIsla( id, bc ); } ).detach();
   }
   Bitacora::Registrar( "INFO", "Mancomunado activo como " + miId + " (" + config.ipPrivada + ":" + std::to_string( config.puertoTcp )
                        + ", UDP " + std::to_string( config.puertoUdp ) + "), " + std::to_string( config.destinos.size() ) + " hilos de isla", "mancomunado" );
   return true;
}

// ---------------------------------------------------------------- UDP (60 / 61)

std::string Mancomunado::MensajeAnnounce() {
   int n = ( ++contador ) % 10000;
   char id[8];
   std::snprintf( id, sizeof( id ), "%04d", n );
   return std::string( "60/" ) + id + "/" + miId + "/" + config.ipPrivada + "/" + std::to_string( config.puertoTcp );
}

void Mancomunado::EnviarAnnounce( const std::string & ip, int puerto ) {
   udp.SendTo( MensajeAnnounce(), ip, puerto );
}

bool Mancomunado::YaVisto( const std::string & clave ) {
   // Un ID_MSG solo cuenta como duplicado dentro de una ventana de 60 s
   std::lock_guard<std::mutex> lock( mtxVistos );
   std::time_t ahora = std::time( nullptr );
   auto it = vistos.find( clave );
   if ( it != vistos.end() && ahora - it->second < 60 ) return true;
   vistos[clave] = ahora;
   if ( vistos.size() > 512 ) {
      for ( auto i = vistos.begin(); i != vistos.end(); ) {
         if ( ahora - i->second >= 60 ) i = vistos.erase( i );
         else ++i;
      }
   }
   return false;
}

void Mancomunado::HiloUdp() {
   char buf[512];
   while ( activo ) {
      std::string ipOrigen;
      int puertoOrigen = 0;
      long n = udp.RecvFrom( buf, sizeof( buf ) - 1, ipOrigen, puertoOrigen );
      if ( n <= 0 ) continue;                      // timeout
      if ( EmpiezaCon( config.ipPrivada, PREFIJO_RED_PRIVADA ) && !EmpiezaCon( ipOrigen, PREFIJO_RED_PRIVADA ) ) continue;   // solo islas
      std::string msg( buf, n );
      QuitarFinLinea( msg );
      std::vector<std::string> p = Partir( msg, '/' );
      if ( p.empty() ) continue;

      if ( p[0] == "60" && p.size() == 5 ) {
         // 60/ID_MSG/id_isla/ip/puerto
         if ( !std::regex_match( p[1], RE_ID_MSG ) || !std::regex_match( p[2], RE_ID_ISLA ) ||
              !std::regex_match( p[3], RE_IP ) || !std::regex_match( p[4], RE_PUERTO ) ) {
            Bitacora::Registrar( "ERROR", "ANNOUNCE mal formado de " + ipOrigen + ": " + msg, "mancomunado" );
            continue;
         }
         if ( p[2] == miId ) continue;                    // eco de mi propio anuncio
         if ( YaVisto( p[2] + "/" + p[1] ) ) continue;    // duplicado
         int puerto = std::atoi( p[4].c_str() );
         if ( puerto <= 0 || puerto > 65535 ) continue;
         if ( islas.Registrar( p[2], p[3], puerto ) ) {
            Bitacora::Registrar( "REGISTRO", "Intermediario descubierto: " + p[2] + " (" + p[3] + ":" + p[4] + ")", p[2] );
            // Respuesta directa para que la otra isla me conozca sin esperar su siguiente ciclo
            EnviarAnnounce( ipOrigen, puertoOrigen );
         }
      } else if ( p[0] == "61" && p.size() == 2 ) {
         if ( std::regex_match( p[1], RE_ID_ISLA ) && p[1] != miId ) {
            if ( islas.MarcarInactiva( p[1] ) ) {
               Bitacora::Registrar( "INACTIVO", "Intermediario se despidio: " + p[1], p[1] );
            }
         }
      } else {
         Bitacora::Registrar( "ERROR", "Mensaje UDP no reconocido de " + ipOrigen + ": " + msg, "mancomunado" );
      }
   }
}

void Mancomunado::Despedirse() {
   // 61/id_isla: por UDP al broadcast de cada isla y por TCP a las islas conocidas
   std::string bye = "61/" + miId;
   for ( const auto & d : config.destinos ) {
      try { udp.SendTo( bye, d.second, config.puertoUdpIslas ); } catch ( const std::exception & ) {}
   }
   for ( const InfoIsla & i : islas.Todas() ) {
      if ( !i.activa ) continue;
      std::string resp;
      Pedir( i, bye, resp );
   }
   Bitacora::Registrar( "INFO", "ANNOUNCE_BYE enviado", "mancomunado" );
}

// ---------------------------------------------------------------- un hilo por isla

void Mancomunado::HiloIsla( std::string id, std::string broadcast ) {
   while ( activo ) {
      try { EnviarAnnounce( broadcast, config.puertoUdpIslas ); } catch ( const std::exception & ) {}

      InfoIsla isla;
      if ( islas.Obtener( id, isla ) && isla.activa ) {
         std::vector<std::string> cats;
         if ( ConsultarCategorias( isla, cats ) ) {
            islas.ActualizarCategorias( id, cats );
         } else if ( islas.MarcarInactiva( id ) ) {
            Bitacora::Registrar( "INACTIVO", "Intermediario sin respuesta, ruta inhabilitada: " + id, id );
         }
      }
      for ( int i = 0; i < config.segundos && activo; i++ ) {
         std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
      }
   }
}

// ---------------------------------------------------------------- TCP: servidor (10 / 20 / 61)

void Mancomunado::HiloTcp() {
   while ( activo ) {
      try {
         Socket cliente = escuchaTcp.Accept();
         std::thread( &Mancomunado::AtenderTcp, this, std::move( cliente ) ).detach();
      } catch ( const std::exception & e ) {
         Bitacora::Registrar( "ERROR", std::string( "Mancomunado Accept: " ) + e.what(), "mancomunado" );
         std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
      }
   }
}

void Mancomunado::AtenderTcp( Socket cliente ) {
   try {
      cliente.SetReadTimeout( 5 );
      char buf[4096];
      size_t n = cliente.Read( buf, sizeof( buf ) - 1 );
      if ( n == 0 ) return;
      std::string msg( buf, n );
      QuitarFinLinea( msg );
      std::string resp = Responder( msg );
      cliente.Write( resp.data(), resp.size() );
   } catch ( const std::exception & ) {
      // conexion rota o sin datos: no hay a quien responder
   }
}

std::string Mancomunado::Responder( const std::string & m ) {
   if ( m.size() < 2 || !std::isdigit( (unsigned char)m[0] ) || !std::isdigit( (unsigned char)m[1] ) ||
        ( m.size() > 2 && m[2] != '/' ) ) {
      return "02/100";                                      // mensaje mal formado
   }
   std::string tipo = m.substr( 0, 2 );
   std::vector<std::string> p = Partir( m, '/' );

   if ( tipo == "10" ) {                                    // GET_CATEGORIES
      if ( p.size() != 1 ) return "02/100";
      std::string lista;
      std::set<std::string> yaPuestas;
      for ( const std::string & c : servicio.CategoriasLocales() ) {
         std::string nombre = NombreProtocolo( c );
         if ( nombre.empty() || nombre.size() > 50 || !yaPuestas.insert( nombre ).second ) continue;
         lista += ( lista.empty() ? "" : "," ) + nombre;
      }
      return "01/" + lista;
   }

   if ( tipo == "20" ) {                                    // GET_PRODUCTS
      if ( p.size() != 2 ) return "02/100";
      std::string cat = Minusculas( p[1] );
      if ( !std::regex_match( cat, RE_NOMBRE ) ) return "02/102";
      std::vector<FilaProducto> filas;
      bool existe = false;
      for ( const std::string & c : servicio.CategoriasLocales() ) {
         if ( NombreProtocolo( c ) != cat ) continue;
         existe = true;
         servicio.ProductosLocales( c, filas );
      }
      if ( !existe ) return "03/" + cat;
      std::string lista;
      int cuenta = 0;
      for ( const FilaProducto & f : filas ) {
         std::string nombre = NombreProtocolo( f.descripcion );
         if ( nombre.empty() || nombre.size() > 50 ) continue;
         lista += ( cuenta ? ";" : "" ) + nombre + "," + f.precio + "," + f.cantidad;
         cuenta++;
      }
      return "01/" + std::to_string( cuenta ) + "/" + lista;
   }

   if ( tipo == "61" ) {                                    // ANNOUNCE_BYE por TCP
      if ( p.size() != 2 || !std::regex_match( p[1], RE_ID_ISLA ) ) return "02/100";
      if ( islas.MarcarInactiva( p[1] ) ) {
         Bitacora::Registrar( "INACTIVO", "Intermediario se despidio: " + p[1], p[1] );
      }
      return "01";
   }

   return "02/101";                                         // tipo no soportado (50 y 40 pendientes)
}

// ---------------------------------------------------------------- TCP: cliente

bool Mancomunado::Pedir( const InfoIsla & isla, const std::string & mensaje, std::string & respuesta ) const {
   try {
      Socket s( 's', false );
      s.SetReadTimeout( 5 );
      s.Connect( isla.host.c_str(), isla.puerto );
      s.Write( mensaje.data(), mensaje.size() );
      respuesta.clear();
      char buf[4096];
      while ( true ) {
         size_t n = 0;
         try {
            n = s.Read( buf, sizeof( buf ) );
         } catch ( const std::exception & ) {
            if ( respuesta.empty() ) throw;               // timeout sin nada
            break;                                        // respondio pero no cerro la conexion
         }
         if ( n == 0 ) break;
         respuesta.append( buf, n );
      }
      QuitarFinLinea( respuesta );
      return !respuesta.empty();
   } catch ( const std::exception & ) {
      return false;
   }
}

bool Mancomunado::ConsultarCategorias( const InfoIsla & isla, std::vector<std::string> & categorias ) const {
   std::string r;
   if ( !Pedir( isla, "10", r ) ) return false;
   if ( r.compare( 0, 3, "01/" ) != 0 ) return false;
   categorias.clear();
   for ( const std::string & c : Partir( r.substr( 3 ), ',' ) ) {
      if ( std::regex_match( c, RE_NOMBRE ) ) categorias.push_back( c );
   }
   return true;
}

bool Mancomunado::ConsultarProductos( const InfoIsla & isla, const std::string & categoriaProtocolo, std::vector<FilaProducto> & filas ) const {
   std::string r;
   if ( !Pedir( isla, "20/" + categoriaProtocolo, r ) ) return false;
   std::vector<std::string> p = Partir( r, '/' );
   if ( p.size() < 3 || p[0] != "01" ) return false;       // 03 (no existe) o 02 (error)
   for ( const std::string & item : Partir( p[2], ';' ) ) {
      std::vector<std::string> campos = Partir( item, ',' );
      if ( campos.size() != 3 ) continue;
      FilaProducto f;
      f.intermediario = isla.id;
      f.bodega = "-";
      f.categoria = categoriaProtocolo;
      f.descripcion = campos[0];
      f.precio = campos[1];
      f.cantidad = campos[2];
      filas.push_back( f );
   }
   return true;
}