#ifndef Texto_hpp
#define Texto_hpp

#include <cctype>
#include <string>

// Quita espacios de los lados y pone todo en minisculas
inline std::string Normalizar( const std::string & s ) {
   size_t a = s.find_first_not_of( " \t\r\n" );
   if ( a == std::string::npos ) return "";
   size_t b = s.find_last_not_of( " \t\r\n" );
   std::string r = s.substr( a, b - a + 1 );
   for ( char & c : r ) {
      c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
   }
   return r;
}

inline bool IgualesSinMayusculas( const std::string & a, const std::string & b ) {
   return Normalizar( a ) == Normalizar( b );
}

#endif