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

#include <iostream>
#include "Logger.hpp"

Logger::Logger(std::string dir) {
   this->logs = std::ofstream(dir.c_str(), std::ios::app);
}

Logger::~Logger() {
   this->logs.close();
}

void Logger::loadTime() {
   this->tiempo = std::time(nullptr);
   this->tm = std::localtime(&tiempo);
   std::strftime(this->fecha, sizeof(this->fecha), "%Y-%m-%d %H:%M:%S", this->tm);
}

std::string Logger::nombreNodo(Nodo t) {
   switch (t) {
      case Cliente:            return "Cliente";
      case Intermediario:      return "Intermediario";
      case ServidorProductos:  return "ServidorProductos";
      default:                 return "Usuario";
   }
}

void Logger::log(std::string txt, Nodo t) {
   std::lock_guard<std::mutex> guard(this->mtx);

   this->loadTime();
   std::string nodo = this->nombreNodo(t);

   std::string t_print;
   for (char c : txt) {
      if (c == '\n') {
         t_print += "\\n [>] ";
      } else if (c == '\r') {
         t_print += "\\r [>] ";
      } else {
         t_print += c;
      }
   }

   this->logs << "[" << this->fecha << "] [" << nodo << "]: ";
   this->logs << "\n\t[>]\t" << t_print.c_str() << std::endl;
}

void Logger::logv(std::vector<std::string>& txt, Nodo t) {
   std::lock_guard<std::mutex> guard(this->mtx);

   this->loadTime();
   std::string nodo = this->nombreNodo(t);

   std::string t_print;
   for (std::string s : txt) {
      t_print += "\n\t[>]\t";
      t_print += s;
   }

   this->logs << "[" << this->fecha << "] [" << nodo << "]: ";
   this->logs << t_print.c_str() << std::endl;
}
