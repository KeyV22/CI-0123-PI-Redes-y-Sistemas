 /**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Bitacora.h: Definición de la clase Bitacora
  *
  * (Fedora version)
  *
 **/

#ifndef BITACORA_H
#define BITACORA_H

// Bibliotecas incluidas
#include <chrono>
#include <ctime>
#include <string>
#include <fstream>
#include <iostream>

// Clase Bitacora
class Bitacora {
  public:
    // Método constructor de la clase Bitacora
    Bitacora(std::string);
    // Método destructor de la clase Bitacora
    ~Bitacora();
    // Método utilizado para registrar acciones dentro del programa
    void registrarAccion(std::string, std::string);

  private:
    // Directorio del archivo a utilizar para registrar movimientos
    const std::string ruta;
    // Objeto fstream que permite la lectura y escritura de datos
    std::fstream archivo;

    // Método que devuelve la fecha actual del sistema
    std::string obtenerFechaActual();
};

#endif
