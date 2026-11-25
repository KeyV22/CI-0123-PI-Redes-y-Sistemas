/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  ColaMensajes.cc: Implementación del envoltorio de una cola de mensajes System V
  *
  * (Fedora version)
  *
 **/

#include "ColaMensajes.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>

ColaMensajes::ColaMensajes() {
  // Pide al sistema operativo una cola privada nueva con permisos de lectura y escritura solo para el dueño
  this->idCola = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
  // Si el sistema no pudo crear la cola se informa el motivo
  if (this->idCola == -1) {
    throw std::runtime_error(std::string("msgget: ") + std::strerror(errno));
  }
}

void ColaMensajes::encolar(const Mensaje &mensaje, long tipo) {
  // Paquete que se enviará al sistema operativo
  Paquete paquete;
  // Asigna el tipo del mensaje, que es la dirección del destinatario
  paquete.tipo = tipo;
  // Copia el identificador del cliente
  paquete.idCliente = mensaje.idCliente;
  // Copia el comando
  paquete.comando = mensaje.comando;
  // Copia el contenido con límite de tamaño, dejando siempre espacio para el fin de cadena
  std::strncpy(paquete.contenido, mensaje.contenido, TAMANIO_MAXIMO - 1);
  // Asegura que el contenido termine en fin de cadena
  paquete.contenido[TAMANIO_MAXIMO - 1] = '\0';
  // Calcula los bytes realmente usados: los campos fijos más el contenido con su fin de cadena
  size_t bytesUsados = offsetof(Paquete, contenido) - sizeof(long) + std::strlen(paquete.contenido) + 1;
  // Envía solo los bytes usados para no llenar la cola con relleno vacío
  while (msgsnd(this->idCola, &paquete, bytesUsados, 0) == -1) {
    // Si el error no fue una interrupción por señal se informa el motivo
    if (errno != EINTR) {
      throw std::runtime_error(std::string("msgsnd: ") + std::strerror(errno));
    }
  }
}

Mensaje ColaMensajes::desencolar(long tipo) {
  // Paquete donde se recibirán los datos
  Paquete paquete;
  // Cantidad de bytes recibidos
  ssize_t bytesRecibidos;
  // Espera un mensaje del tipo indicado; se reintenta si una señal interrumpe la llamada
  do {
    // Bloquea hasta que llegue un mensaje cuyo tipo sea el solicitado
    bytesRecibidos = msgrcv(this->idCola, &paquete, sizeof(Paquete) - sizeof(long), tipo, 0);
  } while (bytesRecibidos == -1 && errno == EINTR);
  // Si falló por otro motivo se informa el error
  if (bytesRecibidos == -1) {
    throw std::runtime_error(std::string("msgrcv: ") + std::strerror(errno));
  }
  // Mensaje que se retornará
  Mensaje mensaje;
  // Limpia el mensaje para no devolver basura en el contenido
  std::memset(&mensaje, 0, sizeof(Mensaje));
  // Copia el identificador del cliente
  mensaje.idCliente = paquete.idCliente;
  // Copia el comando
  mensaje.comando = paquete.comando;
  // Copia el contenido con límite de tamaño
  std::strncpy(mensaje.contenido, paquete.contenido, TAMANIO_MAXIMO - 1);
  // Retorna el mensaje recibido
  return mensaje;
}

void ColaMensajes::eliminar() {
  // Pide al sistema operativo que borre la cola y despierte a quien siga esperando en ella
  msgctl(this->idCola, IPC_RMID, nullptr);
}
