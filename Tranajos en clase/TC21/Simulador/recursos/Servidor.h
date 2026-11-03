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
#include "ColaMensajes.h"
#include "SistemaArchivos.h"
#include "Utilidades.h"
#include <functional>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <unistd.h>

// Clase Servidor: cada servidor corre como un proceso independiente y se comunica por colas System V
class Servidor {
  public:
    // Método constructor que recibe las colas, su identificador, la ruta de su archivo de bodegas propio, la bitácora y la función que carga sus bodegas iniciales
    Servidor(ColaMensajes &, ColaMensajes &, long, const std::string &, Bitacora &, std::function<void(SistemaArchivos &)>);
    // Método que crea el proceso del servidor (fork) y comienza a atender solicitudes
    void iniciar();
    // Método que espera la finalización del proceso del servidor, que termina al recibir FIN
    void esperar();

  private:
    // Cola de la que el servidor recibe las solicitudes del intermediario (tipo = idServidor)
    ColaMensajes &colaDesdeIntermediario;
    // Cola por la que el servidor envía sus respuestas al intermediario (tipo 1)
    ColaMensajes &colaHaciaIntermediario;
    // Identificador del servidor, también es el tipo de los mensajes que le corresponden
    long idServidor;
    // Ruta del archivo de bodegas propio de este servidor
    std::string rutaBodegas;
    // Referencia a la bitácora del proyecto
    Bitacora &bitacora;
    // Función que carga las bodegas y productos iniciales; la define Principal.cc
    std::function<void(SistemaArchivos &)> preparar;
    // Sistema de archivos propio de este proceso; se abre dentro del proceso hijo, después del fork
    SistemaArchivos sistemaArchivos;
    // Identificador del proceso hijo (-1 si no está activo)
    pid_t pid;

    // Nombre del servidor para la bitácora
    std::string nombre();
    // Método que permite procesar la solicitud enviada por el intermediario
    Mensaje procesarSolicitud(const Mensaje &);
    // Método para responder ante una solicitud inválida
    Mensaje mensajeInvalido(const Mensaje &);
    // Busca un producto en todas las bodegas; llena bodegaEncontrada y productoEncontrado si lo halla
    bool buscarEnTodasLasBodegas(const std::string &nombreProducto, ProductoTexto &productoEncontrado);
    // Indica si una bodega existe en el sistema de archivos del servidor
    bool bodegaExiste(const std::string &);
    // Indica si un producto ya existe dentro de una bodega
    bool productoEnBodega(const std::string &, const std::string &);
    // Método que construye la respuesta para la acción filtrar
    Mensaje filtrar(Mensaje &, const std::vector<std::string> &, const std::string &, const std::string &);
    // Método que construye la respuesta para la acción actualizar producto
    Mensaje actualizarProducto(Mensaje &, const std::string &, const std::string &, const std::string &, const std::string &);
    // Método que construye la respuesta para la acción imprimir categorías
    Mensaje imprimirCategorias(Mensaje &, std::vector<std::string> &);
    // Método que construye la respuesta para la acción imprimir productos
    Mensaje imprimirProductos(Mensaje &, std::string &, std::string &);
    // Métodos que atienden cada comando del protocolo relacionado con productos/categorías
    Mensaje atenderBuscarProducto(const Mensaje &);
    Mensaje atenderCategorias(const Mensaje &);
    Mensaje atenderProductos(const Mensaje &);
    Mensaje atenderExistencia(const Mensaje &);
    Mensaje atenderFiltrar(const Mensaje &);
    Mensaje atenderCrearProducto(const Mensaje &);
    Mensaje atenderActualizarProducto(const Mensaje &);
    Mensaje atenderEliminarProducto(const Mensaje &);
    // Ciclo principal que ejecuta el proceso hijo
    void ejecutar();
};

#endif
