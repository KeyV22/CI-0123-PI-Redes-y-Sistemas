/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Servidor.h: Definición de la clase Servidor
  *
  * (Fedora version)
  *
 **/
 
#ifndef SERVIDOR_H
#define SERVIDOR_H

// Bibliotecas incluidas
#include "Bitacora.h"
#include "Buzon.h"
#include "SistemaArchivos.h"
#include "Utilidades.h"
#include "ProtocoloIslas.h"
#include <thread>
#include <atomic>
#include <cstring>
#include <sstream>

// Clase Servidor
class Servidor {
  public:
    // Método constructor de la clase Servidor
    Servidor(Buzon &, Buzon &, SistemaArchivos &, Bitacora &);
    // Método destructor de la clase Servidor
    ~Servidor();
    // Método que permite iniciar el hilo del servidor
    void iniciar();
    // Método que permite detener el hilo del servidor
    void detener();
    // Cambia la vigencia de las reservas del protocolo entre islas (por defecto 60 segundos)
    void fijarVigenciaReserva(int);

  private:
    // Buzón en el que se encuentran las respuestas creadas para el intermediario
    Buzon &buzonHaciaIntermediario;
    // Buzón en el que se encuentran las solicitudes enviadas desde el intermediario
    Buzon &buzonDesdeIntermediario;
    // Referencia al sistema de archivos donde se administran las bodegas
    SistemaArchivos &sistemaArchivos;
    // Referencia a la bitácora del proyecto
    Bitacora &bitacora;
    // Lógica del protocolo mancomunado entre islas
    ProtocoloIslas protocolo;
    // Variable atómica definida para almacenar el estado actual del servidor
    std::atomic<Estado> estado;
    // Hilo definido para ejecutar el servidor
    std::thread hilo;

    // Método que permite procesar la solicitud enviada por el intermediario
    Mensaje procesarSolicitud(const Mensaje &);
    // Método para responder ante una solicitud inválida
    Mensaje mensajeInvalido(const Mensaje &);
    // Busca un producto en todas las bodegas; llena bodegaEncontrada y productoEncontrado si lo halla
    bool buscarEnTodasLasBodegas(const std::string &nombreProducto, ProductoTexto &productoEncontrado);
    // Métodos que atienden cada comando del protocolo relacionado con productos/categorías
    Mensaje atenderBuscarProducto(const Mensaje &);
    Mensaje atenderCategorias(const Mensaje &);
    Mensaje atenderProductos(const Mensaje &);
    Mensaje atenderExistencia(const Mensaje &);
    Mensaje atenderFiltrar(const Mensaje &);
    Mensaje atenderCrearProducto(const Mensaje &);
    Mensaje atenderActualizarProducto(const Mensaje &);
    Mensaje atenderEliminarProducto(const Mensaje &);
    // Atiende un mensaje del protocolo entre islas
    Mensaje atenderProtocolo(const Mensaje &);
    // Método que ejecuta el hilo del servidor
    void ejecutar();
};

#endif
