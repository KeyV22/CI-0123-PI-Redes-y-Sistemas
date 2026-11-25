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
#include <cstring>
#include "Mensaje.h"

// Método que separa una cadena en varios elementos utilizando un separador definido
std::vector<std::string> separarCampos(const std::string &texto, char separador);
// Método que separa una cadena en varios elementos conservando los campos vacíos (incluido el último)
std::vector<std::string> separarConVacios(const std::string &texto, char separador);
// Reemplaza cada aparición de un separador por otro carácter (por defecto un espacio)
std::string reemplazarSeparador(const std::string &texto, char separadorOriginal, char separadorNuevo = ' ');
// Indica si un texto ya se encuentra dentro de un vector de textos
bool contiene(const std::vector<std::string> &textos, const std::string &texto);
// Retorna el nombre textual que el protocolo le da a un comando enviado por un cliente
std::string nombreComando(Comando comando);
// Convierte un texto a minúsculas
std::string aMinusculas(std::string texto);
// Convierte un precio en centavos a texto con dos decimales exactos (por ejemplo 1250 -> "12.50")
std::string formatearPrecio(long centavos);
// Copia un texto en el contenido de un mensaje sin desbordarlo; si no cabe, lo corta y siempre deja fin de cadena
void copiarTexto(char *destino, const std::string &origen);
// Copia una lista de elementos separados por un carácter
bool copiarLista(char *destino, const std::string &lista, char separador);

#endif
