#include "Utilidades.h"

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

std::string reemplazarSeparador(const std::string &texto, char separadorOriginal) {
  // Copia local del texto para modificar
  std::string resultado = texto;
  // Recorre el texto reemplazando cada aparición del separador por un espacio
  for (char &caracter : resultado) {
    if (caracter == separadorOriginal) { caracter = ' '; }
  }
  return resultado;
}
