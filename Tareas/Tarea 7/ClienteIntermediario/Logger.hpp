/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  TicAmazon - Bitacora de eventos del sistema cliente <-> intermediario <-> productos
  *
 **/

#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <fstream>
#include <string>
#include <ctime>
#include <vector>
#include <mutex>

enum Nodo {
   Cliente,
   Intermediario,
   ServidorProductos,
   Usuario
};

class Logger {
   private:
      std::ofstream logs;
      std::time_t tiempo;
      std::tm* tm;
      char fecha[80];
      std::mutex mtx;      // el servidor con hilos escribe bitacora desde varios hilos a la vez

   public:
      Logger(std::string dir);
      ~Logger();

      void log(std::string txt, Nodo t = Cliente);
      void logv(std::vector<std::string>& txt, Nodo t);

   private:
      void loadTime();
      std::string nombreNodo(Nodo t);
};

#endif
