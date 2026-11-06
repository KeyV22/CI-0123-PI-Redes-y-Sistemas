/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Cliente.cc: Implementación de la clase Cliente (proceso independiente)
  *
  * (Fedora version)
  *
 **/

#include "Cliente.h"

Cliente::Cliente(ColaMensajes &colaHaciaIntermediario, ColaMensajes &colaDesdeIntermediario, int idCliente, const std::vector<Mensaje> &solicitudes, Bitacora &bitacora) :
    colaHaciaIntermediario(colaHaciaIntermediario),
    colaDesdeIntermediario(colaDesdeIntermediario),
    idCliente(idCliente),
    solicitudes(solicitudes),
    bitacora(bitacora),
    pid(-1) {}

std::string Cliente::nombre() {
  // Nombre con el que el cliente se identifica en la bitácora y en la salida
  return "Cliente " + std::to_string(this->idCliente);
}

void Cliente::iniciar() {
  // Vacía la salida pendiente para que el proceso hijo no la repita
  std::cout << std::flush;
  // Crea el proceso hijo que ejecutará el cliente
  this->pid = fork();
  // Si no se pudo crear el proceso se informa el error
  if (this->pid == -1) {
    std::cerr << "No se pudo crear el proceso del " << this->nombre() << std::endl;
  // Código que solo ejecuta el proceso hijo
  } else if (this->pid == 0) {
    // Ejecuta todas las solicitudes del cliente
    try {
      // Ejecuta el ciclo del proceso
      this->ejecutar();
    // Si falla una cola se informa y se termina el proceso con error
    } catch (const std::exception &excepcion) {
      std::cerr << this->nombre() << ": " << excepcion.what() << std::endl;
      _exit(1);
    }
    // Termina el proceso hijo sin ejecutar destructores que afecten recursos del padre
    _exit(0);
  }
}

void Cliente::esperar() {
  // Si el proceso fue creado, espera a que termine para no dejarlo como zombi
  if (this->pid > 0) {
    // Espera la finalización del proceso hijo
    waitpid(this->pid, nullptr, 0);
    // Indica que ya no hay un proceso activo
    this->pid = -1;
  }
}

Mensaje Cliente::consultar(const Mensaje &solicitud) {
  // Se encola la solicitud para el intermediario, que siempre las espera con tipo 1
  this->colaHaciaIntermediario.encolar(solicitud, 1);
  // Registro de bitácora: encolamiento de mensaje
  this->bitacora.registrarAccion(this->nombre(), "encolamiento de mensaje");
  // Se espera la respuesta dirigida a este cliente
  Mensaje respuesta = this->colaDesdeIntermediario.desencolar(this->idCliente);
  // Registro de bitácora: desencolamiento de mensaje
  this->bitacora.registrarAccion(this->nombre(), "desencolamiento de mensaje");
  // Se retorna la respuesta recibida
  return respuesta;
}

void Cliente::mostrar(const Mensaje &solicitud, const Mensaje &respuesta) {
  // Texto de la solicitud que muestra el comando y, si tiene, su contenido
  std::string pedido = nombreComando(solicitud.comando);
  // Se agrega el contenido solo si no está vacío, para no dejar espacios de más
  if (solicitud.contenido[0] != '\0') { pedido += " " + std::string(solicitud.contenido); }
  // Se arma una sola línea para que no se mezcle con la salida de otros clientes
  std::string linea = "[" + this->nombre() + "] " + pedido + " -> " + respuesta.contenido + "\n";
  // Se imprime la línea completa y se vacía la salida de inmediato
  std::cout << linea << std::flush;
}

void Cliente::ejecutar() {
  // Registro de bitácora: inicialización
  this->bitacora.registrarAccion(this->nombre(), "inicialización");
  // Se realiza cada solicitud, una tras otra
  for (Mensaje solicitud : this->solicitudes) {
    // Se identifica la solicitud con el cliente que la envía
    solicitud.idCliente = this->idCliente;
    // Se envía la solicitud y se espera la respuesta
    Mensaje respuesta = this->consultar(solicitud);
    // Se muestra el resultado en pantalla
    this->mostrar(solicitud, respuesta);
    // Registro de bitácora: mensaje recibido
    this->bitacora.registrarAccion(this->nombre(), "mensaje recibido");
  }
  // Registro de bitácora: finalización
  this->bitacora.registrarAccion(this->nombre(), "finalización");
}
