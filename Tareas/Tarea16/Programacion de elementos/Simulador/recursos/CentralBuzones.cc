#include "CentralBuzones.h"

void CentralBuzones::registrar(int idCliente) {
  // Activa el candado para evitar condiciones de carrera sobre el mapa
  std::lock_guard<std::mutex> bloquear(this->candado);
  // Crea el buzón solo si no existe uno para este id; si ya existe, no hace nada
  this->buzones[idCliente];
}

void CentralBuzones::encolar(int idCliente, const Mensaje &mensaje) {
  // Puntero al buzón correspondiente, obtenido de forma segura
  Buzon *buzon;
  // Zona crítica en la que se busca el buzón dentro del mapa
  {
    // Activa el candado para evitar condiciones de carrera sobre el mapa
    std::lock_guard<std::mutex> bloquear(this->candado);
    // Busca el buzón asociado al id indicado
    buzon = &this->buzones.at(idCliente);
  }
  // Encola el mensaje ya fuera del candado del mapa, usando el candado propio del buzón
  buzon->encolar(mensaje);
}

Mensaje CentralBuzones::desencolar(int idCliente) {
  // Puntero al buzón correspondiente, obtenido de forma segura
  Buzon *buzon;
  // Zona crítica en la que se busca el buzón dentro del mapa
  {
    // Activa el candado para evitar condiciones de carrera sobre el mapa
    std::lock_guard<std::mutex> bloquear(this->candado);
    // Busca el buzón asociado al id indicado
    buzon = &this->buzones.at(idCliente);
  }
  // Desencola el mensaje (bloqueante) ya fuera del candado del mapa
  return buzon->desencolar();
}