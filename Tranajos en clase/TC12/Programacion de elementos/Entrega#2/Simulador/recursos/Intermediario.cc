#include "Intermediario.h"

Intermediario::Intermediario(Buzon &buzonDesdeCliente, Buzon &buzonHaciaServidor, Buzon &buzonDesdeServidor, CentralBuzones &centralBuzones, Bitacora &bitacora) :
    buzonDesdeCliente(buzonDesdeCliente),
    buzonHaciaServidor(buzonHaciaServidor),
    buzonDesdeServidor(buzonDesdeServidor),
    centralBuzones(centralBuzones),
    bitacora(bitacora),
    estado(Estado::Esperando) {}

Intermediario::~Intermediario() {}

void Intermediario::iniciar() {
  // Registro de bitácora: inicialización
  this->bitacora.registrarAccion("Intermediario", "inicialización");
  // Inicializa y ejecuta el hilo del intermediario
  this->hilo = std::thread(&Intermediario::ejecutar, this);
}

void Intermediario::detener() {
  // Se crea un mensaje para finalizar el programa
  Mensaje fin;
  // Se declara la operación "FIN"
  fin.idCliente = FIN;
  // Se indica el comando
  fin.comando = Comando::Fin;
  // Se copia el contenido indicando el final
  strcpy(fin.contenido, "TERMINAR");
  // Se ingresa el mensaje en el buzón
  this->buzonDesdeCliente.encolar(fin);
  // Registro de bitácora: encolamiento de operación final
  this->bitacora.registrarAccion("Intermediario", "encolamiento de operación final");
  // Se cambia el estado del intermediario
  this->estado = Estado::Detenido;
  // Se finaliza el hilo de ejecución
  if (this->hilo.joinable()) {
    this->hilo.join();
  }
}

Mensaje Intermediario::procesarSolicitud(const Mensaje &mensaje) {
  // Copia base de la respuesta
  Mensaje respuesta = mensaje;
  // Variable local donde se arma el texto final que verá el cliente
  std::string textoFinal;
  // Se traduce el comando de resultado del Servidor al formato textual del protocolo
  if (mensaje.comando == Comando::Ok)
    textoFinal = "OK " + reemplazarSeparador(mensaje.contenido, ':');
  else if (mensaje.comando == Comando::OkCatalogo)
    textoFinal = "OK_CATALOGO";
  else if (mensaje.comando == Comando::Disponible)
    textoFinal = "DISPONIBLE " + reemplazarSeparador(mensaje.contenido, ':');
  else if (mensaje.comando == Comando::Si)
    textoFinal = "SI " + std::string(mensaje.contenido);
  else if (mensaje.comando == Comando::No)
    textoFinal = "NO";
  else if (mensaje.comando == Comando::Coincidencia)
    textoFinal = "COINCIDENCIA " + reemplazarSeparador(mensaje.contenido, ':');
  else if (mensaje.comando == Comando::SinCoincidencia)
    textoFinal = "SIN_COINCIDENCIA";
  else if (mensaje.comando == Comando::Bloqueado)
    textoFinal = "BLOQUEADO " + std::string(mensaje.contenido);
  else if (mensaje.comando == Comando::ErrorFormatoInvalido)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " FORMATO_INVALIDO";
  else if (mensaje.comando == Comando::ErrorSesionInvalida)
    textoFinal = "ERROR SESION_INVALIDA";
  else if (mensaje.comando == Comando::ErrorCategoriaInexistente)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " CATEGORIA_INEXISTENTE";
  else if (mensaje.comando == Comando::ErrorCategoriaExistente)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " CATEGORIA_EXISTENTE";
  else if (mensaje.comando == Comando::ErrorCategoriaNoVacia)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " CATEGORIA_NO_VACIA";
  else if (mensaje.comando == Comando::ErrorBodegaInexistente)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " BODEGA_INEXISTENTE";
  else if (mensaje.comando == Comando::ErrorProductoExistente)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " PRODUCTO_EXISTENTE";
  else if (mensaje.comando == Comando::ErrorNoDisponible)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " NO_DISPONIBLE";
  else if (mensaje.comando == Comando::ErrorPermisoDenegado)
    textoFinal = "ERROR PERMISO_DENEGADO";
  else if (mensaje.comando == Comando::ErrorRecursoBloqueado)
    textoFinal = "ERROR RECURSO_BLOQUEADO";
  else
    textoFinal = "ERROR FORMATO_INVALIDO";
  // Se copia el texto final al contenido del mensaje para el cliente
  strcpy(respuesta.contenido, textoFinal.c_str());
  return respuesta;
}

void Intermediario::ejecutar() {
  // Variable local para guardar el mensaje a enviar
  Mensaje mensaje;
  // Se ejecuta mientras el estado del intermediario no sea detenido
  while (this->estado != Estado::Detenido) {
    // Se declara que el intermediario está esperando
    this->estado = Estado::Esperando;
    // Se recibe la solicitud de cualquier cliente
    Mensaje solicitud = this->buzonDesdeCliente.desencolar();
    // Registro de bitácora: desencolamiento de mensaje
    this->bitacora.registrarAccion("Intermediario", "desencolamiento de mensaje");
    // Se declara que el intermediario está procesando la solicitud del cliente
    this->estado = Estado::Procesando;
    // Si se indica la operación "FIN", finaliza la operación actual
    if (solicitud.idCliente == FIN) { break; }
    // Se encola el mensaje de solicitud para el servidor
    this->buzonHaciaServidor.encolar(solicitud);
    // Registro de bitácora: encolamiento de mensaje
    this->bitacora.registrarAccion("Intermediario", "encolamiento de mensaje");
    // Se declara que el intermediario está esperando
    this->estado = Estado::Esperando;
    // Recibe la respuesta desde el servidor
    Mensaje respuesta = this->buzonDesdeServidor.desencolar();
    // Registro de bitácora: desencolamiento de mensaje
    this->bitacora.registrarAccion("Intermediario", "desencolamiento de mensaje");
    // Se declara que el intermediario está procesando la solicitud del cliente
    this->estado = Estado::Procesando;
    // Se crea la respuesta para el cliente
    mensaje = procesarSolicitud(respuesta);
    // Registro de bitácora: proceso de solicitud
    this->bitacora.registrarAccion("Intermediario", "proceso de solicitud");
    // Se encola la respuesta en el buzón del cliente correspondiente
    this->centralBuzones.encolar(mensaje.idCliente, mensaje);
    // Registro de bitácora: encolamiento de mensaje
    this->bitacora.registrarAccion("Intermediario", "encolamiento de mensaje");
  }
}
