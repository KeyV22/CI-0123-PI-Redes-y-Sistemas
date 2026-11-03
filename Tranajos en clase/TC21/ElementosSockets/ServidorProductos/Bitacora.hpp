#ifndef Bitacora_hpp
#define Bitacora_hpp

#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

class Bitacora {
   public:
      static void Registrar( const std::string & mensaje ) {
         std::string linea = "[" + Ahora() + "] " + mensaje + "\n";

         std::lock_guard<std::mutex> lock( Mutex() );
         std::cout << linea << std::flush;
         std::ofstream archivo( ARCHIVO, std::ios::app );
         if ( archivo.is_open() ) {
            archivo << linea;
         }
      }

   private:
      static constexpr const char * ARCHIVO = "servidor.txt";

      static std::mutex & Mutex() {
         static std::mutex m;
         return m;
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