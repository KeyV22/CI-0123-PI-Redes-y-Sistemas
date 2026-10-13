#include <iostream>
#include <thread>
#include "Servidor.hpp"
#include "Manejadorconexion.hpp"

std::atomic<bool> Servidor::apagar( false );
int Servidor::puertoEscucha = 0;
// El constructor de Socket ya crea el descriptor (socket() se llama
// dentro de VSocket::Init, invocado desde el constructor de Socket).
// 's' = stream (TCP), false = IPv4.
Servidor::Servidor( int puerto ) : puerto( puerto ), escucha( 's', false ), corriendo( false ) {}

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
         // Copy-initialization desde el valor devuelto por Accept():
         // usa el move constructor de Socket (no hace falta un Socket
         // "vacio" previo, y no se crea ningun descriptor de mas).
         Socket cliente = escucha.Accept();
         if ( apagar ) break;   // conexion de despertar: no se atiende

         // Un hilo por conexion: no bloquea el Accept() siguiente mientras
         // se atiende esta. Se separa (detach) porque cada hilo termina y
         // libera sus recursos por su cuenta al cerrar el socket del cliente.
         std::thread hilo( &ManejadorConexion::Atender, std::move( cliente ), std::ref( servicio ) );
         hilo.detach();
      } catch ( const std::exception & e ) {
         // Un Accept() fallido (ej. interrumpido por señal) no debe
         // tumbar el servidor completo: se loguea y se sigue esperando.
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