#ifndef Bitacora_hpp
#define Bitacora_hpp

#include <cctype>
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

struct EventoBitacora {
   long long id = 0;
   std::string fecha;
   std::string tipo;
   std::string origen;
   std::string mensaje;
};

struct ResumenBitacora {
   long long total = 0;                          // TODOS los eventos desde que arranco
   std::map<std::string, long long> porTipo;
   std::map<std::string, long long> porOrigen;
};

class Bitacora {
   public:
      // Compatibilidad: evento sin tipo
      static void Registrar( const std::string & mensaje ) {
         Registrar( "INFO", mensaje, "" );
      }

      static void Registrar( const std::string & tipo, const std::string & mensaje,const std::string & origen = "" ) {
         EventoBitacora ev;
         ev.fecha = Ahora();
         ev.tipo = Mayusculas( tipo );
         ev.origen = origen.empty() ? "intermediario" : origen;
         ev.mensaje = mensaje;

         std::lock_guard<std::mutex> lock( Mutex() );
         Estado & e = Datos();
         ev.id = ++e.resumen.total;
         e.resumen.porTipo[ ev.tipo ]++;
         e.resumen.porOrigen[ ev.origen ]++;
         e.recientes.push_back( ev );
         if ( e.recientes.size() > MAX_RECIENTES ) e.recientes.pop_front();

         std::string linea = "[" + ev.fecha + "] [" + ev.tipo + "] [" + ev.origen + "] " + ev.mensaje + "\n";
         std::cout << linea << std::flush;
         std::ofstream archivo( ARCHIVO, std::ios::app );
         if ( archivo.is_open() ) {
            archivo << linea;
         }
      }

      static std::vector<EventoBitacora> Ultimos( const std::string & filtroTipo, size_t maximo ) {
         std::string f = Mayusculas( filtroTipo );
         std::vector<EventoBitacora> r;
         std::lock_guard<std::mutex> lock( Mutex() );
         const std::deque<EventoBitacora> & rec = Datos().recientes;
         for ( auto it = rec.rbegin(); it != rec.rend() && r.size() < maximo; ++it ) {
            if ( f.empty() || it->tipo == f ) r.push_back( *it );
         }
         return r;
      }

      static ResumenBitacora Resumen() {
         std::lock_guard<std::mutex> lock( Mutex() );
         return Datos().resumen;
      }

   private:
      struct Estado {
         ResumenBitacora resumen;
         std::deque<EventoBitacora> recientes;
      };

      static constexpr const char * ARCHIVO = "servidor.txt";
      static constexpr size_t MAX_RECIENTES = 200;

      static Estado & Datos() {
         static Estado e;
         return e;
      }

      static std::mutex & Mutex() {
         static std::mutex m;
         return m;
      }

      static std::string Mayusculas( std::string s ) {
         for ( char & c : s ) c = (char)std::toupper( (unsigned char)c );
         return s;
      }

      static std::string Ahora() {
         std::time_t t = std::time( nullptr );
         std::tm tmLocal;
         localtime_r( &t, &tmLocal );   // version segura para hilos
         char buf[32];
         std::strftime( buf, sizeof( buf ), "%Y-%m-%d %H:%M:%S", &tmLocal );
         return buf;
      }
};

#endif