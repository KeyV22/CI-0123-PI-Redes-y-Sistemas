// Programa 2: RESPONDE al descubrimiento por broadcast.
// Un HILO espera (RecvFrom) las solicitudes de su contraparte en un puerto UDP
// y contesta (SendTo) en unicast a la IP y puerto de origen del datagrama.
// El hilo principal queda libre: espera Enter para cerrar.
//
//   Uso: ./responder [puertoUDP=9090] [nombre=Servidor-A] [puertoServicio=9091]
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include "Socket.hpp"

static std::atomic<bool> activo( true );

// Cuerpo del hilo: espera solicitudes hasta que 'activo' sea false.
static void EsperarSolicitudes( Socket & udp, const std::string & nombre, int puertoServicio ) {
   char buffer[256];
   while ( activo ) {
      try {
         std::string ipOrigen;
         int puertoOrigen = 0;
         size_t n = udp.RecvFrom( buffer, sizeof( buffer ) - 1, &ipOrigen, &puertoOrigen );
         if ( 0 == n ) {
            continue;   // venció el timeout: se revisa 'activo' y se sigue esperando
         }
         buffer[n] = '\0';
         std::cout << "Solicitud de " << ipOrigen << ":" << puertoOrigen << " -> " << buffer << std::endl;

         if ( std::string( buffer ) != "DISCOVER/SERVIDOR" ) {
            std::cout << "  (ignorada)" << std::endl;
            continue;
         }
         std::string oferta = "OFFER/" + nombre + "/" + std::to_string( puertoServicio );
         udp.SendTo( oferta.c_str(), ipOrigen.c_str(), puertoOrigen );
         std::cout << "  Respondido: " << oferta << std::endl;
      } catch ( const std::exception & e ) {
         std::cerr << "Error en el hilo: " << e.what() << "\n";
      }
   }
}

int main( int argc, char * argv[] ) {
   int puerto = ( argc > 1 ) ? std::atoi( argv[1] ) : 9090;
   std::string nombre = ( argc > 2 ) ? argv[2] : "Servidor-A";
   int puertoServicio = ( argc > 3 ) ? std::atoi( argv[3] ) : 9091;

   try {
      Socket udp( 'd', false );
      udp.Bind( puerto );
      udp.SetReadTimeout( 1 );   // sin esto RecvFrom bloquearia y el hilo no podria terminar

      std::thread hilo( EsperarSolicitudes, std::ref( udp ), nombre, puertoServicio );
      std::cout << "Hilo esperando solicitudes UDP en el puerto " << puerto
                << ". Presione Enter para salir." << std::endl;

      std::cin.get();            // el hilo principal solo espera el Enter
      activo = false;
      hilo.join();
      std::cout << "Cerrado." << std::endl;
   } catch ( const std::exception & e ) {
      std::cerr << "Error: " << e.what() << "\n";
      return 1;
   }
   return 0;
}