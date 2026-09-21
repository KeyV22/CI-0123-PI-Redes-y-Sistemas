#include <iostream>
#include <chrono>
#include <thread>
#include "Descubrimiento.hpp"

int main() {

   Descubrimiento peerA( "SERV", "127.0.0.1", 9091, 5000 );
   Descubrimiento peerB( "INT_04", "127.0.0.1", 8080, 5000 );

   peerA.IniciarEscucha();
   peerB.IniciarEscucha();

   std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );

   peerA.AnunciarPresencia();
   peerB.AnunciarPresencia();

   std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

   InfoPeer info;
   bool encontroA = peerB.Buscar( "SERV", &info );
   std::cout << "peerB conoce a SERV: " << ( encontroA ? "SI" : "NO" );
   if ( encontroA ) std::cout << " (" << info.ip << ":" << info.puerto << ")";
   std::cout << "\n";

   bool encontroB = peerA.Buscar( "INT_04", &info );
   std::cout << "peerA conoce a INT_04: " << ( encontroB ? "SI" : "NO" );
   if ( encontroB ) std::cout << " (" << info.ip << ":" << info.puerto << ")";
   std::cout << "\n";

   std::cout << "\n--- peerA se despide (DEATH) ---\n";
   peerA.AnunciarMuerte();
   std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );

   bool siguePresente = peerB.Buscar( "SERV", &info );
   std::cout << "peerB sigue viendo a SERV: " << ( siguePresente ? "SI (FALLO)" : "NO (correcto)" ) << "\n";

   peerA.DetenerEscucha();
   peerB.DetenerEscucha();

   return ( encontroA && encontroB && !siguePresente ) ? 0 : 1;

}
