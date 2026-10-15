/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Cliente.h: Definición de la clase Cliente
  *
  * (Fedora version)
  *
 **/

#ifndef CLIENTE_H
#define CLIENTE_H

// Bibliotecas incluidas
#include "Bitacora.h"
#include "Buzon.h"
#include "CentralBuzones.h"
#include <cstring>
#include <thread>
#include <atomic>
#include <iostream>

// Clase Cliente
class Cliente {
  public:
    // Método constructor de la clase Cliente
    Cliente(Buzon &, CentralBuzones &, Bitacora &);
    // Método destructor de la clase Cliente
    ~Cliente();
    // Método que permite iniciar el hilo del cliente
    void iniciar(const Mensaje &);
    // Método que permite detener el hilo del cliente
    void detener();

  private:
    // Buzón compartido en el que se encuentran las solicitudes para el intermediario
    Buzon &buzonHaciaIntermediario;
    // Central que direcciona la respuesta al buzón de este cliente
    CentralBuzones &centralBuzones;
    // Referencia a la bitácora del proyecto
    Bitacora &bitacora;
    // Variable atómica definida para almacenar el estado actual del cliente
    std::atomic<Estado> estado;
    // Hilo definido para ejecutar el cliente
    std::thread hilo;

    // Método que ejecuta el hilo del cliente
    void ejecutar(const Mensaje &);
};

#endif
