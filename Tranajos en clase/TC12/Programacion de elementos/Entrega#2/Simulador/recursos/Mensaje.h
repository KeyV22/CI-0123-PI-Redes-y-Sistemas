/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Mensaje.h: Definición de la estructura de mensaje
  *
  * (Fedora version)
  *
 **/

#ifndef MENSAJE_H
#define MENSAJE_H

// Bibliotecas incluídas
#include <string>

// Constante definida para el tamaño máximo del contenido del mensaje
#define TAMANIO_MAXIMO 2048

// Constante definida para señalar el fin de una operación
#define FIN -1

// Constante de tipo string que indica la ruta del sistema de archivos
const std::string RUTA_ALMACENAMIENTO = "bodegas/Bodegas.data";

// Constante de tipo string que indica la ruta de la bitácora
const std::string RUTA_BITACORA = "bitacora/Bitacora.txt";

// Constantes enumeradas definidas para el comando o resultado del mensaje
enum class Comando {
  // Comandos de sesión
  Conectar,
  Desconectar,
  // Comandos Cliente / Intermediario
  Categorias,
  Productos,
  Existencia,
  Filtrar,
  CrearProducto,
  ActualizarProducto,
  EliminarProducto,
  // Comandos Intermediario / Servidor
  BuscarProducto,
  Disponible,
  Enviar,
  ModificarProducto,
  BajaProducto,
  // Resultados exitosos
  Ok,
  OkCatalogo,
  Si,
  No,
  Coincidencia,
  SinCoincidencia,
  Bloqueado,
  // Resultados de error
  ErrorFormatoInvalido,
  ErrorSesionInvalida,
  ErrorCategoriaInexistente,
  ErrorCategoriaExistente,
  ErrorCategoriaNoVacia,
  ErrorBodegaInexistente,
  ErrorProductoExistente,
  ErrorNoDisponible,
  ErrorPermisoDenegado,
  ErrorRecursoBloqueado,
  // Marcador definido para señalar el fin de una operación
  Fin
};

// Constantes enumeradas definidas para determinar el estado actual de la entidad
enum class Estado {
  Esperando,
  Procesando,
  Detenido
};

// Estructura definida para los mensajes entre el cliente y el intermediario
struct Mensaje {
  // Identificador del cliente que envía el mensaje
  int idCliente;
  // Comando definido para el mensaje
  Comando comando;
  // Contenido del mensaje
  char contenido[TAMANIO_MAXIMO];
};

#endif
