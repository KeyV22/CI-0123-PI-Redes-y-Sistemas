#include "Bitacora.h"

Bitacora::Bitacora(std::string ruta) : ruta(ruta) {
  // Inicializa el archivo en donde se escribirá el historial de acciones
  this->archivo = std::fstream(ruta, std::ios::out | std::ios::app);
}

Bitacora::~Bitacora() {
  // Cierra la lectura y escritura en el archivo
  this->archivo.close();
}

void Bitacora::registrarAccion(std::string enteResponsable, std::string accion) {
  // Variable local que obtiene la fecha actual del sistema
  auto fecha = obtenerFechaActual();
  // Evalua si el archivo se encuentra disponible para lectura o edición
  if(this->archivo.is_open()) {
    this->archivo << "[ " << fecha << " ] "
            << "[ " << enteResponsable << " ] " 
            << "[ " << accion << " ]" 
            << std::endl;
  // En caso de que no poderse, se reporta el error
  } else {
    std::cerr << "No se pudo leer el archivo de bitácora" << std::endl;
  }
}

std::string Bitacora::obtenerFechaActual() {
  // Obtiene la fecha del sistema con la librería chrono
  auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  // Búfer en el cual se guarda la fecha obtenida
  char bufer[11];
  // Se realiza un cambio de formato
  std::strftime(bufer, sizeof(bufer), "%Y-%m-%d", std::localtime(&t));
  // Se retorna la fecha como string
  return std::string(bufer);
}
