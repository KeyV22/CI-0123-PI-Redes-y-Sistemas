/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Intermediario2.h: Definición del intermediario de la isla remota que habla el protocolo mancomunado
  *
  * (Fedora version)
  *
 **/

#ifndef INTERMEDIARIO2_H
#define INTERMEDIARIO2_H

// Bibliotecas incluidas
#include "Bitacora.h"
#include "ColaMensajes.h"
#include "Utilidades.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// Constante que indica los segundos de vigencia de una reserva, según el protocolo
static const int VIGENCIA_RESERVA = 60;

// Constantes que contienen las expresiones regulares del protocolo mancomunado
static const std::regex REGEX_TIPO("^[0-9]{2}$");
static const std::regex REGEX_CATEGORIA("^[a-zA-Z0-9_\\-]{1,50}$");
static const std::regex REGEX_PRODUCTO("^[a-zA-Z0-9_\\-]{1,50}$");
static const std::regex REGEX_CANTIDAD("^[0-9]{1,5}$");
static const std::regex REGEX_CONTEO("^[0-9]{1,3}$");

// Clase Intermediario2
class Intermediario2 {
  public:
    // Método constructor que recibe la cola de solicitudes, la cola de respuestas y la bitácora
    Intermediario2(ColaMensajes &, ColaMensajes &, Bitacora &);
    // Método que crea el proceso de la isla remota (fork) y comienza a atender solicitudes
    void iniciar();
    // Método que espera la finalización del proceso, que termina al recibir FIN
    void esperar();

  private:
    // Producto de la isla remota; el precio se guarda en centavos para evitar errores de redondeo
    struct ProductoRemoto {
      // Nombre del producto
      std::string nombre;
      // Precio en centavos
      long centavos;
      // Cantidad disponible
      int existencia;
    };

    // Categoría de la isla remota con sus productos
    struct CategoriaRemota {
      // Nombre de la categoría
      std::string nombre;
      // Productos de la categoría
      std::vector<ProductoRemoto> productos;
    };

    // Reserva vigente de un producto
    struct Reserva {
      // Cantidad reservada
      int cantidad;
      // Momento en que la reserva expira
      time_t vence;
    };

    // Lista ordenada de productos y cantidades de una compra
    typedef std::vector<std::pair<std::string, int>> Pedido;
    // Cantidad total pedida por producto (suma los elementos repetidos)
    typedef std::map<std::string, int> Totales;

    // Cola de la que recibe las solicitudes del intermediario local (tipo 1)
    ColaMensajes &colaDesdeIntermediario;
    // Cola por la que envía sus respuestas al intermediario local (tipo 1)
    ColaMensajes &colaHaciaIntermediario;
    // Referencia a la bitácora del proyecto
    Bitacora &bitacora;
    // Catálogo fijo de la isla remota
    std::vector<CategoriaRemota> catalogo;
    // Reservas vigentes indexadas por nombre de producto
    std::map<std::string, Reserva> reservas;
    // Identificador del proceso hijo (-1 si no está activo)
    pid_t pid;

    // Carga las categorías y productos fijos de la isla remota
    void cargarCatalogo();
    // Ciclo principal que recibe solicitudes, las procesa y responde hasta recibir FIN
    void ejecutar();
    // Interpreta un mensaje del protocolo mancomunado y devuelve la respuesta en texto
    std::string procesar(const std::string &);
    // Método que atiende 10 (GET_CATEGORIES)
    std::string atenderCategorias(const std::vector<std::string> &);
    // Método que atiende 20 (GET_PRODUCTS)
    std::string atenderProductos(const std::vector<std::string> &);
    // Método que atiende 50 (RESERVE)
    std::string atenderReserva(const std::vector<std::string> &);
    // Método que atiende 40 (BUY)
    std::string atenderCompra(const std::vector<std::string> &);
    // Método que valida el formato de n y de la lista y llena el pedido
    std::string interpretarPedido(const std::vector<std::string> &, Pedido &, Totales &);
    // Método que valida un elemento producto, cantidad y extrae sus valores
    std::string interpretarElemento(const std::string &, std::string &, int &);
    // Método que verifica que cada producto exista, tenga reserva vigente y stock
    std::string verificarPedido(const Totales &);
    // Método que descuenta el stock, consume las reservas y construye la factura
    std::string facturarPedido(const Pedido &, const Totales &);
    // Método que busca un producto en todas las categorías
    ProductoRemoto *buscarProducto(const std::string &);
    // Construye la respuesta 02/código
    static std::string error(int);
};

#endif
