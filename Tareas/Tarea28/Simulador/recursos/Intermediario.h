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
#include "ColaMensajes.h"
#include "EnlaceMancomunado.h"
#include "Mensaje.h"
#include "Utilidades.h"
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Clase Intermediario: corre como un proceso independiente y se comunica por colas System V
class Intermediario {
  public:
    // Método constructor que recibe las cuatro colas locales, las dos colas hacia la isla remota, los identificadores de los servidores y la bitácora
    Intermediario(ColaMensajes &, ColaMensajes &, ColaMensajes &, ColaMensajes &, ColaMensajes &, ColaMensajes &, const std::vector<long> &, Bitacora &);
    // Método que crea el proceso del intermediario (fork) y comienza a atender solicitudes
    void iniciar();
    // Método que envía la solicitud de finalización y espera a que termine el proceso del intermediario
    void detener();

  private:
    // Cola de la que se reciben las solicitudes de cualquier cliente (tipo 1)
    ColaMensajes &colaDesdeCliente;
    // Cola por la que se envían las respuestas a los clientes (tipo = idCliente)
    ColaMensajes &colaHaciaCliente;
    // Cola por la que se envían las solicitudes a los servidores (tipo = idServidor)
    ColaMensajes &colaHaciaServidor;
    // Cola de la que se reciben las respuestas de los servidores (tipo 1)
    ColaMensajes &colaDesdeServidor;
    // Identificadores de los servidores de productos disponibles
    std::vector<long> idsServidores;
    // Referencia a la bitácora del proyecto
    Bitacora &bitacora;
    // Enlace con la isla remota, que se consulta con el protocolo mancomunado
    EnlaceMancomunado enlace;
    // Identificadores de los clientes que tienen una sesión abierta
    std::set<int> sesiones;
    // Identificador del proceso hijo (-1 si no está activo)
    pid_t pid;

    // Envía una solicitud a un servidor y espera su respuesta
    Mensaje consultarServidor(long, const Mensaje &);
    // Evalúa el comando de la solicitud y decide cómo se consulta a los servidores
    Mensaje resolverSolicitud(const Mensaje &);
    // Pregunta a los servidores uno por uno hasta que alguno responda con el comando de éxito indicado
    Mensaje buscarEnServidores(const Mensaje &, Comando);
    // Pregunta a todos los servidores y une las listas de las respuestas con el comando de éxito indicado
    Mensaje unirRespuestas(const Mensaje &, Comando);
    // Envía la solicitud a todos los servidores y retorna sus respuestas en orden
    std::vector<Mensaje> consultarTodos(const Mensaje &);
    // Reúne, sin repetir, los elementos de las listas de las respuestas que tienen el comando de éxito indicado
    std::vector<std::string> reunirElementos(const std::vector<Mensaje> &, Comando);
    // Arma la respuesta de éxito con los elementos separados por ':'
    Mensaje armarLista(const Mensaje &, Comando, const std::vector<std::string> &);
    // Elige la primera que no sea de bodega inexistente cuando no hubo elementos
    Mensaje elegirRespuesta(const Mensaje &, const std::vector<Mensaje> &);
    // Pregunta a los servidores uno por uno hasta que alguno tenga la bodega y aplique el cambio
    Mensaje aplicarCambio(const Mensaje &);
    // Busca en los servidores locales y, si no lo encuentra, en la isla remota (BuscarProducto y Existencia)
    Mensaje buscarConRespaldoRemoto(const Mensaje &, Comando);
    // Une las categorías de los servidores locales con las de la isla remota
    Mensaje unirCategorias(const Mensaje &);
    // Lista los productos de una categoría en los servidores locales y, si no existe, en la isla remota
    Mensaje listarProductos(const Mensaje &);
    // Atiende CONNECT abriendo la sesión del cliente
    Mensaje atenderConectar(const Mensaje &);
    // Atiende DISCONNECT en donde cierra la sesión del cliente o responde sesión inválida si no estaba abierta
    Mensaje atenderDesconectar(const Mensaje &);
    // Indica si el contenido de una solicitud especifica una bodega (campo separado por ':'), lo que la limita a los servidores locales
    bool tieneBodega(const Mensaje &);
    // Traduce la respuesta del servidor al formato textual del protocolo
    Mensaje procesarSolicitud(const Mensaje &);
    // Envía la solicitud de finalización a todos los servidores
    void finalizarServidores();
    // Ciclo principal que ejecuta el proceso hijo
    void ejecutar();
};

#endif
