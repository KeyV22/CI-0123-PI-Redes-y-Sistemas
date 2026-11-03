// Principal.cc
/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Principal.cc: Programa central del proyecto
  *
  *  Lanzador de la simulación: crea las colas de mensajes System V y lanza, como procesos
  *  independientes, dos servidores, un intermediario, una isla remota (Intermediario2) y cuatro clientes que se reparten los casos de prueba
  *
  * (Fedora version)
  *
 **/

#include "Cliente.h"
#include "Intermediario.h"
#include "Intermediario2.h"
#include "Servidor.h"
#include "ColaMensajes.h"
#include "SistemaArchivos.h"
#include "Utilidades.h"
#include "Mensaje.h"
#include "Bitacora.h"

#include <iostream>
#include <vector>
#include <sys/stat.h>

// Rutas de los archivos de bodegas propios de cada servidor
const std::string RUTA_SERVIDOR_1 = "bodegas/Bodegas1.data";
const std::string RUTA_SERVIDOR_2 = "bodegas/Bodegas2.data";

// Método que inicializa las bodegas y productos provisionales del servidor 1
void inicializarBodegasPrincipal(SistemaArchivos &sistemaArchivos) {
  // Crea la bodega provisional para las pruebas
  sistemaArchivos.crear_bodega("principal");
  // Carga algunos productos de prueba, organizados por categoría
  sistemaArchivos.insertar_producto("principal", "granos", "garbanzos", "50", "600");
  sistemaArchivos.insertar_producto("principal", "granos", "arroz", "50", "1000");
  sistemaArchivos.insertar_producto("principal", "condimentos", "aceite vegetal", "25", "1000");
  sistemaArchivos.insertar_producto("principal", "condimentos", "sal", "25", "200");
}

// Método que inicializa las bodegas y productos provisionales del servidor 2
void inicializarBodegasSucursal(SistemaArchivos &sistemaArchivos) {
  // Crea la bodega provisional para las pruebas
  sistemaArchivos.crear_bodega("sucursal");
  // Carga algunos productos de prueba, organizados por categoría
  sistemaArchivos.insertar_producto("sucursal", "lacteos", "queso", "15", "2500");
  sistemaArchivos.insertar_producto("sucursal", "lacteos", "yogur", "30", "900");
  sistemaArchivos.insertar_producto("sucursal", "bebidas", "jugo", "40", "1200");
}

// Método que crea una solicitud con el comando y el contenido indicados; el cliente le asigna su identificador
Mensaje crearSolicitud(Comando comando, const std::string &contenido) {
  // Mensaje local para armar la solicitud
  Mensaje solicitud;
  // Se limpia el mensaje para no enviar basura
  std::memset(&solicitud, 0, sizeof(Mensaje));
  // Se indica el comando
  solicitud.comando = comando;
  // Se copia el contenido
  copiarTexto(solicitud.contenido, contenido);
  // Se retorna la solicitud
  return solicitud;
}

// Casos del cliente 1: los que usan el producto "garbanzos", que deben ir en este orden
std::vector<Mensaje> solicitudesCliente1() {
  return {
    // Caso 29: Conectar - abre la sesión del cliente
    crearSolicitud(Comando::Conectar, ""),
    // Caso 1: BuscarProducto - existe
    crearSolicitud(Comando::BuscarProducto, "garbanzos"),
    // Caso 6: Existencia - con bodega indicada, producto existe
    crearSolicitud(Comando::Existencia, "garbanzos:principal"),
    // Caso 13: ActualizarProducto - producto existente
    crearSolicitud(Comando::ActualizarProducto, "garbanzos:principal:60:650"),
    // Caso 14: ActualizarProducto - producto no existe en esa bodega
    crearSolicitud(Comando::ActualizarProducto, "inexistente:principal:10:10"),
    // Caso 25: Productos - categoría que solo existe en la isla remota (protocolo mancomunado)
    crearSolicitud(Comando::Productos, "ropa"),
    // Caso 30: Desconectar - cierra la sesión abierta
    crearSolicitud(Comando::Desconectar, "")
  };
}

// Casos del cliente 2: los que usan los condimentos, que deben ir en este orden
std::vector<Mensaje> solicitudesCliente2() {
  return {
    // Caso 31: Conectar - abre la sesión del cliente
    crearSolicitud(Comando::Conectar, ""),
    // Caso 4: Productos - categoría con productos, en una bodega específica
    crearSolicitud(Comando::Productos, "condimentos:principal"),
    // Caso 5: Productos - categoría inexistente
    crearSolicitud(Comando::Productos, "higiene:principal"),
    // Caso 9: Filtrar - hay coincidencia
    crearSolicitud(Comando::Filtrar, "condimentos:acei:principal"),
    // Caso 10: Filtrar - sin coincidencia
    crearSolicitud(Comando::Filtrar, "condimentos:xyz:principal"),
    // Caso 15: EliminarProducto - producto existente
    crearSolicitud(Comando::EliminarProducto, "aceite vegetal:principal"),
    // Caso 16: EliminarProducto - producto no existe
    crearSolicitud(Comando::EliminarProducto, "noexiste:principal"),
    // Caso 26: BuscarProducto - el producto solo existe en la isla remota
    crearSolicitud(Comando::BuscarProducto, "sandalias"),
    // Caso 32: Desconectar - cierra la sesión abierta
    crearSolicitud(Comando::Desconectar, "")
  };
}

// Casos del cliente 3: los que no dependen de otros casos
std::vector<Mensaje> solicitudesCliente3() {
  return {
    // Caso 33: Desconectar - sin sesión abierta, responde sesión inválida
    crearSolicitud(Comando::Desconectar, ""),
    // Caso 34: Conectar - abre la sesión del cliente
    crearSolicitud(Comando::Conectar, ""),
    // Caso 2: BuscarProducto - no existe
    crearSolicitud(Comando::BuscarProducto, "cantonés"),
    // Caso 3: Categorias - hay categorías registradas, en ambos servidores
    crearSolicitud(Comando::Categorias, ""),
    // Caso 7: Existencia - sin bodega indicada, producto existe
    crearSolicitud(Comando::Existencia, "sal"),
    // Caso 8: Existencia - producto no existe en ninguna bodega
    crearSolicitud(Comando::Existencia, "leche"),
    // Caso 11: CrearProducto - bodega existente
    crearSolicitud(Comando::CrearProducto, "frijoles:granos:principal:40:700"),
    // Caso 12: CrearProducto - bodega inexistente
    crearSolicitud(Comando::CrearProducto, "leche:lacteos:secundaria:10:100"),
    // Caso 17: Comando no manejado por el Servidor (cae en mensajeInvalido)
    crearSolicitud(Comando::Enviar, ""),
    // Caso 27: Existencia - el producto solo existe en la isla remota
    crearSolicitud(Comando::Existencia, "camisa"),
    // Caso 28: BuscarProducto - el producto no existe ni localmente ni en la isla remota
    crearSolicitud(Comando::BuscarProducto, "abrigo"),
    // Caso 35: Desconectar - cierra la sesión abierta
    crearSolicitud(Comando::Desconectar, ""),
    // Caso 36: Desconectar - la sesión ya estaba cerrada, responde sesión inválida
    crearSolicitud(Comando::Desconectar, "")
  };
}

// Casos del cliente 4: los que usan el servidor 2 (bodega "sucursal"), que deben ir en este orden
std::vector<Mensaje> solicitudesCliente4() {
  return {
    // Caso 37: Conectar - abre la sesión del cliente
    crearSolicitud(Comando::Conectar, ""),
    // Caso 18: BuscarProducto - el producto está solo en el servidor 2
    crearSolicitud(Comando::BuscarProducto, "queso"),
    // Caso 19: Existencia - el producto está solo en el servidor 2
    crearSolicitud(Comando::Existencia, "jugo"),
    // Caso 20: Productos - categoría que solo existe en el servidor 2
    crearSolicitud(Comando::Productos, "lacteos"),
    // Caso 21: Filtrar - coincidencia en el servidor 2
    crearSolicitud(Comando::Filtrar, "lacteos:yo"),
    // Caso 22: CrearProducto - bodega del servidor 2
    crearSolicitud(Comando::CrearProducto, "crema:lacteos:sucursal:5:900"),
    // Caso 23: ActualizarProducto - producto del servidor 2
    crearSolicitud(Comando::ActualizarProducto, "queso:sucursal:12:1500"),
    // Caso 24: EliminarProducto - producto del servidor 2
    crearSolicitud(Comando::EliminarProducto, "yogur:sucursal"),
    // Caso 38: Desconectar - cierra la sesión abierta
    crearSolicitud(Comando::Desconectar, "")
  };
}

// Método que ejecuta la simulación completa con las colas ya creadas
void simular(ColaMensajes &colaSolicitudesCliente, ColaMensajes &colaRespuestasCliente, ColaMensajes &colaSolicitudesServidor, ColaMensajes &colaRespuestasServidor, ColaMensajes &colaHaciaRemoto, ColaMensajes &colaDesdeRemoto) {
  // Inicializa la bitácora compartida por todos los procesos
  Bitacora bitacora(RUTA_BITACORA);
  // Crea los dos servidores, cada uno con su identificador, su archivo de bodegas y sus bodegas iniciales
  Servidor servidor1(colaSolicitudesServidor, colaRespuestasServidor, 1, RUTA_SERVIDOR_1, bitacora, inicializarBodegasPrincipal);
  Servidor servidor2(colaSolicitudesServidor, colaRespuestasServidor, 2, RUTA_SERVIDOR_2, bitacora, inicializarBodegasSucursal);
  // Crea la isla remota, que responde el protocolo mancomunado
  Intermediario2 islaRemota(colaHaciaRemoto, colaDesdeRemoto, bitacora);
  // Crea el intermediario, que conoce los identificadores de los dos servidores y el enlace con la isla remota
  Intermediario intermediario(colaSolicitudesCliente, colaRespuestasCliente, colaSolicitudesServidor, colaRespuestasServidor, colaHaciaRemoto, colaDesdeRemoto, {1, 2}, bitacora);
  // Crea los cuatro clientes, cada uno con su identificador y sus solicitudes
  Cliente cliente1(colaSolicitudesCliente, colaRespuestasCliente, 1, solicitudesCliente1(), bitacora);
  Cliente cliente2(colaSolicitudesCliente, colaRespuestasCliente, 2, solicitudesCliente2(), bitacora);
  Cliente cliente3(colaSolicitudesCliente, colaRespuestasCliente, 3, solicitudesCliente3(), bitacora);
  Cliente cliente4(colaSolicitudesCliente, colaRespuestasCliente, 4, solicitudesCliente4(), bitacora);

  // Se inician los servidores, la isla remota y el intermediario
  servidor1.iniciar();
  servidor2.iniciar();
  islaRemota.iniciar();
  intermediario.iniciar();
  // Se inician los clientes haciendo sus solicitudes en paralelo
  cliente1.iniciar();
  cliente2.iniciar();
  cliente3.iniciar();
  cliente4.iniciar();

  // Se espera a que todos los clientes terminen sus solicitudes
  cliente1.esperar();
  cliente2.esperar();
  cliente3.esperar();
  cliente4.esperar();
  // Se detiene el intermediario y pide finalizar a los servidores
  intermediario.detener();
  // Se espera a que los servidores y la isla remota terminen
  servidor1.esperar();
  servidor2.esperar();
  islaRemota.esperar();
}

int main() {
  // Código de salida del programa
  int resultado = 0;
  // Se crean las seis colas de mensajes antes de crear cualquier proceso, para que los hijos las hereden
  ColaMensajes colaSolicitudesCliente;
  ColaMensajes colaRespuestasCliente;
  ColaMensajes colaSolicitudesServidor;
  ColaMensajes colaRespuestasServidor;
  // Colas del enlace mancomunado entre el intermediario y la isla remota
  ColaMensajes colaHaciaRemoto;
  ColaMensajes colaDesdeRemoto;
  try {
    // Se ejecuta la simulación completa
    simular(colaSolicitudesCliente, colaRespuestasCliente, colaSolicitudesServidor, colaRespuestasServidor, colaHaciaRemoto, colaDesdeRemoto);
  // Si algo falla se informa, pero igual se eliminan las colas
  } catch (const std::exception &excepcion) {
    std::cerr << "Error en la simulación: " << excepcion.what() << std::endl;
    resultado = 1;
  }
  // Solo el proceso padre elimina las colas del sistema
  colaSolicitudesCliente.eliminar();
  colaRespuestasCliente.eliminar();
  colaSolicitudesServidor.eliminar();
  colaRespuestasServidor.eliminar();
  colaHaciaRemoto.eliminar();
  colaDesdeRemoto.eliminar();
  return resultado;
}
