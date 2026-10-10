#ifndef PROTOCOLOISLAS_H
#define PROTOCOLOISLAS_H

#include "Bitacora.h"
#include "SistemaArchivos.h"
#include "Utilidades.h"
#include <chrono>
#include <map>
#include <set>
#include <string>
#include <vector>

// Protocolo mancomunado entre islas (lado de la isla).
// Recibe un mensaje (60, 61, 10, 20, 50, 40) y devuelve la respuesta (01, 02 o 03).
// Un texto vacío significa que el mensaje no genera respuesta (UDP: 60 y 61).
class ProtocoloIslas {
  public:
    ProtocoloIslas(SistemaArchivos &, Bitacora &);
    std::string procesar(const std::string &);
    // Cambia la vigencia de las reservas (por defecto 60 segundos)
    void fijarVigenciaReserva(int);

  private:
    struct Reserva {
      std::string producto;
      int cantidad;
      std::chrono::steady_clock::time_point vence;
    };
    struct Isla {
      std::string ip;
      std::string puerto;
    };

    SistemaArchivos &sistemaArchivos;
    Bitacora &bitacora;
    int vigenciaSegundos;
    std::vector<Reserva> reservas;
    std::map<std::string, Isla> islas;     // islas conocidas por ANNOUNCE
    std::set<std::string> idsVistos;       // ID_MSG ya vistos, para descartar duplicados

    std::string atenderAnnounce(const std::vector<std::string> &);
    std::string atenderAnnounceBye(const std::vector<std::string> &);
    std::string atenderCategorias(const std::vector<std::string> &);
    std::string atenderProductos(const std::vector<std::string> &);
    std::string atenderReserva(const std::vector<std::string> &);
    std::string atenderCompra(const std::vector<std::string> &);

    static std::string error(const std::string &);
    static std::string noEncontrado(const std::string &);
    static std::string minusculas(std::string);
    static std::string decimal(double);
    static bool valido(const std::string &, const char *);
    bool buscarProducto(const std::string &, ProductoTexto &);
    void descartarVencidas();
    int reservado(const std::string &);
    void consumirReserva(const std::string &, int);
    int existencia(const ProductoTexto &);
    void cambiarExistencia(const ProductoTexto &, int);
};

#endif