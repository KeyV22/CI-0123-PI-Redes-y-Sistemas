/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Bitacora.h: Definición de la bitácora compartida entre los procesos de la simulación
  *
  * (Fedora version)
  *
 **/

#ifndef BITACORA_H
#define BITACORA_H

// Bibliotecas incluidas
#include <cerrno>
#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <unistd.h>

// Clase Bitacora
class Bitacora {
  public:
    // Método constructor de la clase Bitacora el cual abre el archivo antes de crear los procesos para que los hijos lo hereden
    Bitacora(std::string);
    // Método destructor de la clase Bitacora
    ~Bitacora();
    // Método utilizado para registrar acciones dentro del programa el cual recibe la entidad responsable y la acción realizada
    void registrarAccion(std::string, std::string);

  private:
    // Directorio del archivo a utilizar para registrar movimientos
    const std::string ruta;
    // Descriptor del archivo de bitácora
    int descriptor;

    // Método que devuelve la fecha y la hora actuales del sistema con milisegundos
    std::string obtenerMarcaTiempo();
};

#endif
