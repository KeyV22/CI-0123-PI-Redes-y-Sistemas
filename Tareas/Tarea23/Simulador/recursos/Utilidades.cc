#include "Utilidades.h"
#include <algorithm>
#include <cctype>
#include <cstdio>

std::vector<std::string> separarCampos(const std::string &texto, char separador) {
  // Vector local para acumular los campos encontrados
  std::vector<std::string> campos;
  // Guarda el texto en un stringstream para recorrerlo
  std::stringstream flujo(texto);
  // Variable local para guardar cada campo leído
  std::string campo;
  // Recorre el texto separando por el separador indicado
  while (std::getline(flujo, campo, separador)) {
    campos.push_back(campo);
  }
  return campos;
}

std::vector<std::string> separarConVacios(const std::string &texto, char separador) {
  // Vector local para acumular los campos encontrados
  std::vector<std::string> campos;
  // Campo que se está armando
  std::string actual;
  // Recorre el texto carácter por carácter
  for (char caracter : texto) {
    // Si es el separador se cierra el campo actual, aunque esté vacío
    if (caracter == separador) {
      campos.push_back(actual);
      actual.clear();
    // Si no, el carácter pertenece al campo actual
    } else {
      actual += caracter;
    }
  }
  // Se agrega el último campo, que también puede ser vacío
  campos.push_back(actual);
  return campos;
}

std::string reemplazarSeparador(const std::string &texto, char separadorOriginal, char separadorNuevo) {
  // Copia local del texto para modificar
  std::string resultado = texto;
  // Recorre el texto reemplazando cada aparición del separador original por el nuevo
  for (char &caracter : resultado) {
    if (caracter == separadorOriginal) { caracter = separadorNuevo; }
  }
  return resultado;
}

void copiarTexto(char *destino, const std::string &origen) {
  // Cantidad de caracteres que caben en el contenido, dejando un espacio para el fin de cadena
  size_t cantidad = origen.size() < TAMANIO_MAXIMO - 1 ? origen.size() : TAMANIO_MAXIMO - 1;
  // Copia solo los caracteres que caben
  std::memcpy(destino, origen.c_str(), cantidad);
  // Agrega el fin de cadena
  destino[cantidad] = '\0';
}

bool copiarLista(char *destino, const std::string &lista, char separador) {
  // Si la lista completa cabe, se copia tal cual
  if (lista.size() < TAMANIO_MAXIMO) {
    copiarTexto(destino, lista);
    return true;
  }
  // Se busca el último separador que deja todos los elementos anteriores dentro del límite
  size_t corte = lista.rfind(separador, TAMANIO_MAXIMO - 1);
  // Si hay un separador se copian solo los elementos completos que caben
  if (corte != std::string::npos && corte > 0) {
    copiarTexto(destino, lista.substr(0, corte));
  // Si ni siquiera el primer elemento cabe, se corta
  } else {
    copiarTexto(destino, lista);
  }
  // La lista quedó incompleta
  return false;
}

bool contiene(const std::vector<std::string> &textos, const std::string &texto) {
  // Recorre los textos del vector
  for (const std::string &actual : textos) {
    // Si alguno coincide, el texto ya está en el vector
    if (actual == texto) { return true; }
  }
  // No se encontró el texto
  return false;
}

std::string nombreComando(Comando comando) {
  // Se traduce el comando al nombre textual del protocolo
  if (comando == Comando::Conectar)
    return "CONNECT";
  else if (comando == Comando::Desconectar)
    return "DISCONNECT";
  else if (comando == Comando::Categorias)
    return "CATEGORIAS";
  else if (comando == Comando::Productos)
    return "PRODUCTOS";
  else if (comando == Comando::Existencia)
    return "EXISTENCIA";
  else if (comando == Comando::Filtrar)
    return "FILTRAR";
  else if (comando == Comando::CrearProducto)
    return "CREAR_PRODUCTO";
  else if (comando == Comando::ActualizarProducto)
    return "ACTUALIZAR_PRODUCTO";
  else if (comando == Comando::EliminarProducto)
    return "ELIMINAR_PRODUCTO";
  else if (comando == Comando::BuscarProducto)
    return "BUSCAR_PRODUCTO";
  else
    return "DESCONOCIDO";
}

std::string aMinusculas(std::string texto) {
  // Convierte cada carácter a minúscula
  std::transform(texto.begin(), texto.end(), texto.begin(), [](unsigned char caracter) { return std::tolower(caracter); });
  return texto;
}

std::string formatearPrecio(long centavos) {
  // Buffer para el texto
  char buffer[32];
  // Formatea con dos decimales exactos
  std::snprintf(buffer, sizeof(buffer), "%ld.%02ld", centavos / 100, centavos % 100);
  return std::string(buffer);
}
