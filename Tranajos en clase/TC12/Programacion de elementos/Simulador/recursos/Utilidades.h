/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Utilidades.h: utilidades de uso común para el proyecto
  *
  * (Fedora version)
  *
 **/

#ifndef UTILIDADES_H
#define UTILIDADES_H

// Bibliotecas incluídas
#include <sstream>
#include <string>
#include <vector>

// Método que separa una cadena en varios elementos utilizando un separador definido
std::vector<std::string> separarCampos(const std::string &texto, char separador);
// Une los campos de un texto separado por un carácter, usando espacios en su lugar
std::string reemplazarSeparador(const std::string &texto, char separadorOriginal);

#endif
