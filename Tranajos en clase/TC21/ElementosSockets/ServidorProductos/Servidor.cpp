#include <iostream>
#include <thread>
#include "Servidor.hpp"
#include "Manejadorconexion.hpp"
#include <memory>

std::atomic<bool> Servidor::apagar( false );
int Servidor::puertoEscucha = 0;
// El constructor de Socket ya crea el descriptor (socket() se llama
// dentro de VSocket::Init, invocado desde el constructor de Socket).
// 's' = stream (TCP), false = IPv4.
Servidor::Servidor( int puerto, const char * certificado, const char * llave )
   : puerto( puerto ), escucha( certificado, llave ), corriendo( false ) {}

bool Servidor::Iniciar() {
   try {
      escucha.Bind( puerto );
      escucha.Listen( 16 );   // backlog de 16 conexiones pendientes
   } catch ( const std::exception & e ) {
      std::cerr << "No se pudo iniciar el servidor: " << e.what() << "\n";
      return false;
   }

   corriendo = true;
   puertoEscucha = puerto;
   std::cout << "Servidor escuchando en el puerto " << puerto << "\n";
   return true;
}

void Servidor::Ejecutar( ServicioProductos & servicio ) {
   while ( corriendo && !apagar) {
      try {
         std::unique_ptr<SSLSocket> cliente( escucha.AcceptConnection() );
         if ( apagar ) break;
         cliente->CopyContext( &escucha );

         std::thread hilo( &ManejadorConexion::Atender, cliente.get(), std::ref( servicio ) );
         cliente.release();
         hilo.detach();
      } catch ( const std::exception & e ) {
         std::cerr << "[Servidor] Accept() fallo: " << e.what() << "\n";
      }
   }
}

void Servidor::SolicitarApagado() {
   apagar = true;
   try {
      Socket despertar( 's', false );
      despertar.Connect( "127.0.0.1", puertoEscucha );
   } catch ( const std::exception & ) {
      // si no conecta, el servidor termina en el proximo Accept()
   }
}

void Servidor::Detener() {
   corriendo = false;
   escucha.Close();
}