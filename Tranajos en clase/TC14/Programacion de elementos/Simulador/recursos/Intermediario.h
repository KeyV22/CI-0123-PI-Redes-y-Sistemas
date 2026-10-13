/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Intermediario.h: Definición de la clase Intermediario
  *
  * (Fedora version)
  *
 **/

#ifndef INTERMEDIARIO_H
#define INTERMEDIARIO_H

// Bibliotecas incluidas
#include "Bitacora.h"
#include "Buzon.h"
#include "CentralBuzones.h"
#include "Mensaje.h"
#include "Utilidades.h"
#include <cstring>
#include <thread>
#include <atomic>
#include <sstream>

// Clase Intermediario
class Intermediario {
  public:
    // Método constructor de la clase Intermediario
    Intermediario(Buzon &, Buzon &, Buzon &, CentralBuzones &, Bitacora &);
    // Método destructor de la clase Intermediario
    ~Intermediario();
    // Método que permite iniciar el hilo del intermediario
    void iniciar();
    // Método que permite detener el hilo del intermediario
    void detener();

  private:
    // Buzón en el que se encuentran las solicitudes de cualquier cliente
    Buzon &buzonDesdeCliente;
    // Buzón en el que se encuentran las solicitudes para el servidor
    Buzon &buzonHaciaServidor;
    // Buzón en el que se encuentran las respuestas recibidas desde el servidor
    Buzon &buzonDesdeServidor;
    // Central que direcciona las respuestas al buzón del cliente correspondiente
    CentralBuzones &centralBuzones;
    // Referencia a la bitácora del proyecto
    Bitacora &bitacora;
    // Variable atómica definida para almacenar el estado actual del intermediario
    std::atomic<Estado> estado;
    // Hilo definido para ejecutar el intermediario
    std::thread hilo;

    // Método que permite procesar la solicitud enviada por el servidor
    Mensaje procesarSolicitud(const Mensaje &);
    // Método que ejecuta el hilo del intermediario
    void ejecutar();
};

#endif
