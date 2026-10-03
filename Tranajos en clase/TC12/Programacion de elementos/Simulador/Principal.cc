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
  *  Arma manualmente los buzones, la central de buzones, el sistema de archivos,
  *  un Intermediario real y un servidor
  *
  * (Fedora version)
  *
 **/

#include "Cliente.h"
#include "Intermediario.h"
#include "Servidor.h"
#include "Buzon.h"
#include "CentralBuzones.h"
#include "SistemaArchivos.h"
#include "Utilidades.h"
#include "Mensaje.h"
#include "Bitacora.h"

#include <iostream>
#include <vector>

// Método que inicializa las bodegas y productos provisionales para las pruebas
void inicializarBodegas(SistemaArchivos &sistemaArchivos) {
  // Crea la bodega provisional para las pruebas
  sistemaArchivos.crear_bodega("principal");
  // Carga algunos productos de prueba, organizados por categoría
  sistemaArchivos.insertar_producto("principal", "granos", "garbanzos", "50", "600");
  sistemaArchivos.insertar_producto("principal", "granos", "arroz", "50", "1000");
  sistemaArchivos.insertar_producto("principal", "condimentos", "aceite vegetal", "25", "1000");
  sistemaArchivos.insertar_producto("principal", "condimentos", "sal", "25", "200");
}

// Método que testeará el proyecto
void test() {
  // Crea el buzón compartido de solicitudes de cualquier cliente hacia el intermediario
  Buzon buzonHaciaIntermediario;
  // Crea los buzones fijos entre el intermediario y el servidor
  Buzon buzonHaciaServidor;
  Buzon buzonDesdeServidor;
  // Crea la central que direcciona las respuestas al buzón de cada cliente
  CentralBuzones centralBuzones;

  // Inicializa el sistema de archivos, partiendo siempre limpio para pruebas
  SistemaArchivos sistemaArchivos;
  sistemaArchivos.crear_archivo(RUTA_ALMACENAMIENTO);
  // Prepara las bodegas y productos de prueba
  inicializarBodegas(sistemaArchivos);

  // Inicializa el mensaje a procesar
  Mensaje mensaje;
  mensaje.idCliente = 1;

  // Inicializa la bitácora y las entidades
  Bitacora bitacora(RUTA_BITACORA);
  Cliente cliente(buzonHaciaIntermediario, centralBuzones, bitacora);
  Intermediario intermediario(buzonHaciaIntermediario, buzonHaciaServidor, buzonDesdeServidor, centralBuzones, bitacora);
  Servidor servidor(buzonDesdeServidor, buzonHaciaServidor, sistemaArchivos, bitacora);

  // Comienza la ejecución
  intermediario.iniciar();
  servidor.iniciar();

  // Caso 1: BuscarProducto - existe
  mensaje.comando = Comando::BuscarProducto;
  strcpy(mensaje.contenido, "garbanzos");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 2: BuscarProducto - no existe
  mensaje.comando = Comando::BuscarProducto;
  strcpy(mensaje.contenido, "cantonés");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 3: Categorias - hay categorías registradas
  mensaje.comando = Comando::Categorias;
  strcpy(mensaje.contenido, "");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 4: Productos - categoría con productos, en una bodega específica
  mensaje.comando = Comando::Productos;
  strcpy(mensaje.contenido, "condimentos:principal");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 5: Productos - categoría inexistente
  mensaje.comando = Comando::Productos;
  strcpy(mensaje.contenido, "higiene:principal");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 6: Existencia - con bodega indicada, producto existe
  mensaje.comando = Comando::Existencia;
  strcpy(mensaje.contenido, "garbanzos:principal");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 7: Existencia - sin bodega indicada, producto existe
  mensaje.comando = Comando::Existencia;
  strcpy(mensaje.contenido, "sal");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 8: Existencia - producto no existe en ninguna bodega
  mensaje.comando = Comando::Existencia;
  strcpy(mensaje.contenido, "leche");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 9: Filtrar - hay coincidencia
  mensaje.comando = Comando::Filtrar;
  strcpy(mensaje.contenido, "condimentos:acei:principal");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 10: Filtrar - sin coincidencia
  mensaje.comando = Comando::Filtrar;
  strcpy(mensaje.contenido, "condimentos:xyz:principal");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 11: CrearProducto - bodega existente
  mensaje.comando = Comando::CrearProducto;
  strcpy(mensaje.contenido, "frijoles:granos:principal:40:700");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 12: CrearProducto - bodega inexistente
  mensaje.comando = Comando::CrearProducto;
  strcpy(mensaje.contenido, "leche:lacteos:secundaria:10:100");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 13: ActualizarProducto - producto existente
  mensaje.comando = Comando::ActualizarProducto;
  strcpy(mensaje.contenido, "garbanzos:principal:60:650");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 14: ActualizarProducto - producto no existe en esa bodega
  mensaje.comando = Comando::ActualizarProducto;
  strcpy(mensaje.contenido, "inexistente:principal:10:10");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 15: EliminarProducto - producto existente
  mensaje.comando = Comando::EliminarProducto;
  strcpy(mensaje.contenido, "aceite vegetal:principal");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 16: EliminarProducto - producto no existe
  mensaje.comando = Comando::EliminarProducto;
  strcpy(mensaje.contenido, "noexiste:principal");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Caso 17: Comando no manejado por el Servidor (cae en mensajeInvalido)
  mensaje.comando = Comando::Conectar;
  strcpy(mensaje.contenido, "");
  cliente.iniciar(mensaje);
  cliente.detener();

  // Se detienen las entidades
  servidor.detener();
  intermediario.detener();
  // Cierra el sistema de archivos
  sistemaArchivos.cerrar_archivo();
}

int main() {
  test();
  return 0;
}
