#include "Buzon.h"
 
void Buzon::encolar(const Mensaje &mensaje) {
  // Zona crítica en la que se agregan nuevos mensajes al buzón
  {
    // Activa el candado para evitar condiciones de carrera
    std::lock_guard<std::mutex> bloquear(this->candado);
    // Agrega el mensaje entrante al buzón
    this->bufer.push(mensaje);
  }
  // Notifica al hilo consumidor que hay un mensaje disponible
  this->hayElementos.notify_one();
}
 
Mensaje Buzon::desencolar() {
  // Activa el candado para evitar condiciones de carrera
  std::unique_lock<std::mutex> bloquear(this->candado);
  // Se pone en espera el hilo consumidor hasta que haya un mensaje disponible y devuelve el acceso al buzón
  this->hayElementos.wait(bloquear, [this] { return !this->bufer.empty(); });
  // Toma el primer mensaje del buzón
  Mensaje mensaje = this->bufer.front();
  // Quita el mensaje del buzón
  this->bufer.pop();
  // Retorna el mensaje entrante
  return mensaje;
}