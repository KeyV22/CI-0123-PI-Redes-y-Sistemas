/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  ColaMensajes.h: Definición del envoltorio de una cola de mensajes System V
  *
  * (Fedora version)
  *
 **/

#ifndef COLAMENSAJES_H
#define COLAMENSAJES_H

// Bibliotecas incluidas
#include "Mensaje.h"

// Clase ColaMensajes
class ColaMensajes {
  public:
    // Crea una cola de mensajes System V nueva y privada
    ColaMensajes();
    // Guarda un mensaje en la cola dirigido al destinatario indicado por tipo > 0
    void encolar(const Mensaje &mensaje, long tipo);
    // Bloquea al proceso hasta que haya un mensaje para el tipo indicado y lo retorna
    Mensaje desencolar(long tipo);
    // Elimina la cola del sistema. Solo debe llamarla el proceso padre al terminar la simulación
    void eliminar();

  private:
    // Formato del mensaje tal y como lo exige System V: primero el tipo (long) y luego los datos
    struct Paquete {
      // Tipo del mensaje, funciona como dirección del destinatario
      long tipo;
      // Identificador del cliente que originó la solicitud
      int idCliente;
      // Comando o resultado del mensaje
      Comando comando;
      // Contenido textual del mensaje
      char contenido[TAMANIO_MAXIMO];
    };

    // Identificador de la cola que entrega el sistema operativo
    int idCola;
};

#endif
