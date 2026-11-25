/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Bitacora.cc: Implementación de la bitácora compartida entre los procesos de la simulación
  *
  * (Fedora version)
  *
 **/

#include "Bitacora.h"

Bitacora::Bitacora(std::string ruta) : ruta(ruta), descriptor(-1) {
  // Abre el archivo solo para escritura al final
  this->descriptor = open(this->ruta.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
  // Si no se pudo abrir se informa una sola vez
  if (this->descriptor == -1) {
    std::cerr << "No se pudo abrir el archivo de bitácora" << std::endl;
  }
}

Bitacora::~Bitacora() {
  // Si el archivo está abierto se cierra los procesos hijos terminan con _exit() y no ejecutan este destructor
  if (this->descriptor != -1) {
    close(this->descriptor);
  }
}

void Bitacora::registrarAccion(std::string enteResponsable, std::string accion) {
  // Si el archivo no está disponible no hay dónde registrar
  if (this->descriptor == -1) {
    return;
  }
  // Arma la línea completa en memoria para escribirla de una sola vez
  std::string linea = "[ " + this->obtenerMarcaTiempo() + " ] "
    + "[ " + enteResponsable + " ] "
    + "[ PID " + std::to_string(getpid()) + " ] "
    + "[ " + accion + " ]\n";
  // Un único write() por línea el cual solo se repite si una señal lo interrumpe antes de escribir
  ssize_t escritos;
  do {
    escritos = write(this->descriptor, linea.c_str(), linea.size());
  } while (escritos == -1 && errno == EINTR);
}

std::string Bitacora::obtenerMarcaTiempo() {
  // Obtiene el momento actual con precisión de nanosegundos
  struct timespec ahora;
  clock_gettime(CLOCK_REALTIME, &ahora);
  // Estructura donde se guarda la fecha desglosada; localtime_r no usa memoria compartida
  struct tm fecha;
  localtime_r(&ahora.tv_sec, &fecha);
  // Búfer en el cual se guarda la fecha y hora sin milisegundos
  char bufer[32];
  // Se realiza un cambio de formato
  std::strftime(bufer, sizeof(bufer), "%Y-%m-%d %H:%M:%S", &fecha);
  // Búfer del texto final con milisegundos
  char final[48];
  // Se agregan los milisegundos
  std::snprintf(final, sizeof(final), "%s.%03ld", bufer, ahora.tv_nsec / 1000000);
  // Se retorna la marca de tiempo como string
  return std::string(final);
}
