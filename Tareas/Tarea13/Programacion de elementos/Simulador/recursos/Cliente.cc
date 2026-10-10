#include "Cliente.h"

Cliente::Cliente(Buzon &buzonHaciaIntermediario, CentralBuzones &centralBuzones, Bitacora &bitacora) :
    buzonHaciaIntermediario(buzonHaciaIntermediario),
    centralBuzones(centralBuzones),
    bitacora(bitacora),
    estado(Estado::Esperando) {}

Cliente::~Cliente() {}

void Cliente::iniciar(const Mensaje &mensaje) {
  // Registro de bitácora: inicialización
  this->bitacora.registrarAccion("Cliente", "inicialización");
  // Inicializa y ejecuta el hilo del Cliente
  this->hilo = std::thread(&Cliente::ejecutar, this, mensaje);
}

void Cliente::detener() {
  // Registro de bitácora: detención
  this->bitacora.registrarAccion("Cliente", "detención");
  // Cambia el estado del Cliente a detenido
  this->estado = Estado::Detenido;
  // Espera a que el hilo del Cliente termine su ejecución
  if (this->hilo.joinable()) {
    this->hilo.join();
  }
}

void Cliente::ejecutar(const Mensaje &solicitud) {
  // Se declara que el Cliente está procesando la solicitud del cliente
  this->estado = Estado::Procesando;
  // Registra el buzón propio de este cliente antes de enviar nada, para que la respuesta siempre tenga dónde llegar
  this->centralBuzones.registrar(solicitud.idCliente);
  // Se encola el mensaje creado para el intermediario
  this->buzonHaciaIntermediario.encolar(solicitud);
  // Registro de bitácora: encolamiento de mensaje
  this->bitacora.registrarAccion("Cliente", "encolamiento de mensaje");
  // Se declara que el cliente está esperando su respuesta
  this->estado = Estado::Esperando;
  // Recibe la respuesta desde el buzón propio de este cliente
  Mensaje respuesta = this->centralBuzones.desencolar(solicitud.idCliente);
  // Registro de bitácora: desencolamiento de mensaje
  this->bitacora.registrarAccion("Cliente", "desencolamiento de mensaje");

  if (respuesta.comando == Comando::RespuestaProtocolo && respuesta.contenido[0] == '\0') {
    // Los mensajes UDP (60 y 61) no tienen respuesta
    std::cout << "(sin respuesta: mensaje UDP)" << std::endl;
  } else {
    std::cout << respuesta.contenido << std::endl;
  }
  
  // Registro de bitácora: mensaje recibido
  this->bitacora.registrarAccion("Cliente", "mensaje recibido");
  // Detiene la ejecución del cliente
  this->estado = Estado::Detenido;
}
