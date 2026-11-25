/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  EnlaceMancomunado.h: Definición del enlace del intermediario local con la isla remota (protocolo mancomunado)
  *
  * (Fedora version)
  *
 **/

#ifndef ENLACEMANCOMUNADO_H
#define ENLACEMANCOMUNADO_H

// Bibliotecas incluidas
#include "Bitacora.h"
#include "ColaMensajes.h"
#include "Utilidades.h"
#include <cstring>
#include <string>
#include <utility>
#include <vector>

// Clase EnlaceMancomunado
class EnlaceMancomunado {
  public:
    // Producto que la isla remota informa en la respuesta de GET_PRODUCTS
    struct ProductoIsla {
      // Nombre del producto
      std::string nombre;
      // Precio tal y como lo informa la isla (con punto decimal)
      std::string precio;
      // Cantidad disponible
      int existencia;
    };
    // Lista de productos y cantidades de una compra
    typedef std::vector<std::pair<std::string, int>> Compra;

    // Método constructor que recibe la cola hacia la isla remota, la cola desde la isla remota y la bitácora
    EnlaceMancomunado(ColaMensajes &, ColaMensajes &, Bitacora &);
    // Método que llena las categorías de la isla GET_CATEGORIES (10)
    bool obtenerCategorias(std::vector<std::string> &);
    // Método que llena los productos de la categoría indicada GET_PRODUCTS (20)
    bool obtenerProductos(const std::string &, std::vector<ProductoIsla> &);
    // Busca un producto en todas las categorías de la isla
    bool buscarProducto(const std::string &, std::string &, ProductoIsla &);
    // Método que retorna true si la isla respondió 01 RESERVE (50)
    bool reservar(const std::string &, int, std::string &);
    // Método que retorna true si la isla respondió 01 BUY (40): 
    bool comprar(const Compra &, std::string &);
    // Método que envía FIN a la isla remota para que su proceso termine
    void finalizar();

  private:
    // Cola por la que se envían los mensajes a la isla remota (tipo 1)
    ColaMensajes &colaHaciaRemoto;
    // Cola de la que se reciben las respuestas de la isla remota (tipo 1)
    ColaMensajes &colaDesdeRemoto;
    // Referencia a la bitácora del proyecto
    Bitacora &bitacora;

    // Envía un texto del protocolo a la isla remota y retorna su respuesta en texto
    std::string consultar(const std::string &);
    // Indica si una respuesta es un ACK (empieza con el código 01)
    static bool esAck(const std::string &);
    // Interpreta el detalle producto,precio,stock;... de una respuesta 01 de GET_PRODUCTS
    static std::vector<ProductoIsla> interpretarProductos(const std::string &);
};

#endif
