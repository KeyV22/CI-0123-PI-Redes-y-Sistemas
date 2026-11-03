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
#include "ColaMensajes.h"
#include "Mensaje.h"
#include "Utilidades.h"
#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Clase Cliente: cada cliente corre como un proceso independiente y se comunica por colas System V
class Cliente {
  public:
    // Método constructor que recibe las colas, su identificador, la lista de solicitudes que realizará y la bitácora
    Cliente(ColaMensajes &, ColaMensajes &, int, const std::vector<Mensaje> &, Bitacora &);
    // Método que crea el proceso del cliente y comienza a realizar sus solicitudes
    void iniciar();
    // Método que espera a que el proceso del cliente termine todas sus solicitudes
    void esperar();

  private:
    // Cola por la que se envían las solicitudes al intermediario (tipo 1)
    ColaMensajes &colaHaciaIntermediario;
    // Cola de la que se reciben las respuestas del intermediario (tipo = idCliente)
    ColaMensajes &colaDesdeIntermediario;
    // Identificador de este cliente, también es el tipo de los mensajes que le corresponden
    int idCliente;
    // Solicitudes que este cliente realizará una tras otra
    std::vector<Mensaje> solicitudes;
    // Referencia a la bitácora del proyecto
    Bitacora &bitacora;
    // Identificador del proceso hijo (-1 si no está activo)
    pid_t pid;

    // Nombre del cliente para la bitácora y la salida
    std::string nombre();
    // Envía una solicitud al intermediario y espera su respuesta
    Mensaje consultar(const Mensaje &);
    // Muestra en pantalla la solicitud realizada y la respuesta recibida
    void mostrar(const Mensaje &, const Mensaje &);
    // Ciclo principal que ejecuta el proceso hijo
    void ejecutar();
};

#endif
