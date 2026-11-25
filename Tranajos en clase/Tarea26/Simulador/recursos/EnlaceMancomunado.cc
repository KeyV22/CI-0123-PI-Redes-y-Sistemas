/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  EnlaceMancomunado.cc: Implementación del enlace con la isla remota (protocolo mancomunado)
  *
  * (Fedora version)
  *
 **/

#include "EnlaceMancomunado.h"

EnlaceMancomunado::EnlaceMancomunado(ColaMensajes &colaHaciaRemoto, ColaMensajes &colaDesdeRemoto, Bitacora &bitacora) :
    colaHaciaRemoto(colaHaciaRemoto),
    colaDesdeRemoto(colaDesdeRemoto),
    bitacora(bitacora) {}

std::string EnlaceMancomunado::consultar(const std::string &texto) {
  // Mensaje que se envía a la isla remota
  Mensaje solicitud;
  // Se limpia el mensaje para no enviar basura
  std::memset(&solicitud, 0, sizeof(Mensaje));
  // El protocolo mancomunado no lleva cliente de origen
  solicitud.idCliente = 0;
  // Se marca como mensaje mancomunado
  solicitud.comando = Comando::Mancomunado;
  // Se copia el texto del protocolo sin desbordar el contenido
  copiarTexto(solicitud.contenido, texto);
  // Registro de bitácora: encolamiento hacia la isla remota
  this->bitacora.registrarAccion("Intermediario", "encolamiento mancomunado " + texto.substr(0, 2));
  // Se envía la solicitud a la isla remota
  this->colaHaciaRemoto.encolar(solicitud, 1);
  // Se espera la respuesta de la isla remota
  Mensaje respuesta = this->colaDesdeRemoto.desencolar(1);
  // Registro de bitácora: desencolamiento desde la isla remota
  this->bitacora.registrarAccion("Intermediario", "desencolamiento mancomunado");
  // Se retorna el texto de la respuesta
  return std::string(respuesta.contenido);
}

bool EnlaceMancomunado::esAck(const std::string &respuesta) {
  // Un ACK es exactamente "01" o empieza con "01/"
  return respuesta == "01" || respuesta.compare(0, 3, "01/") == 0;
}

bool EnlaceMancomunado::obtenerCategorias(std::vector<std::string> &categorias) {
  // Se envía GET_CATEGORIES
  std::string respuesta = this->consultar("10");
  // Si la isla no respondió 01 no hay categorías
  if (!esAck(respuesta)) { return false; }
  // Se separa 01/categoria1,categoria2,...
  std::vector<std::string> partes = separarCampos(respuesta, '/');
  // Una respuesta sin lista no aporta categorías
  if (partes.size() < 2) { return false; }
  // Se separan las categorías por coma
  categorias = separarCampos(partes[1], ',');
  return true;
}

std::vector<EnlaceMancomunado::ProductoIsla> EnlaceMancomunado::interpretarProductos(const std::string &detalle) {
  // Productos interpretados
  std::vector<ProductoIsla> productos;
  // Cada producto viene como nombre,precio,stock separados por ';'
  for (const std::string &elemento : separarCampos(detalle, ';')) {
    // Se separan los campos del producto
    std::vector<std::string> campos = separarCampos(elemento, ',');
    // Un elemento incompleto se ignora
    if (campos.size() == 3) {
      // Se guarda el producto con su existencia como número
      productos.push_back({campos[0], campos[1], std::stoi(campos[2])});
    }
  }
  // Se retornan los productos
  return productos;
}

bool EnlaceMancomunado::obtenerProductos(const std::string &categoria, std::vector<ProductoIsla> &productos) {
  // Se envía GET_PRODUCTS con la categoría
  std::string respuesta = this->consultar("20/" + categoria);
  // Si la isla respondió 03 o 02 no hay productos
  if (!esAck(respuesta)) { return false; }
  // Se separa 01/cantidad/detalle
  std::vector<std::string> partes = separarCampos(respuesta, '/');
  // Una respuesta sin detalle no aporta productos
  if (partes.size() < 3) { return false; }
  // Se interpretan los productos
  productos = interpretarProductos(partes[2]);
  return true;
}

bool EnlaceMancomunado::buscarProducto(const std::string &nombre, std::string &categoria, ProductoIsla &producto) {
  // Categorías de la isla remota
  std::vector<std::string> categorias;
  // Sin categorías no hay dónde buscar
  if (!this->obtenerCategorias(categorias)) { return false; }
  // Se revisa cada categoría de la isla
  for (const std::string &actual : categorias) {
    // Productos de la categoría actual
    std::vector<ProductoIsla> productos;
    // Si la categoría no se pudo consultar se sigue con la siguiente
    if (!this->obtenerProductos(actual, productos)) { continue; }
    // Se busca el producto por nombre
    for (const ProductoIsla &candidato : productos) {
      if (candidato.nombre == nombre) {
        // Se informa la categoría y el producto encontrados
        categoria = actual;
        producto = candidato;
        return true;
      }
    }
  }
  // Ninguna categoría de la isla tiene el producto
  return false;
}

bool EnlaceMancomunado::reservar(const std::string &producto, int cantidad, std::string &respuesta) {
  // Se envía RESERVE y se guarda el texto completo de la isla
  respuesta = this->consultar("50/" + producto + "/" + std::to_string(cantidad));
  // Éxito solo si la isla respondió 01
  return esAck(respuesta);
}

bool EnlaceMancomunado::comprar(const Compra &compra, std::string &respuesta) {
  // Detalle producto,cantidad;producto,cantidad;...
  std::string detalle;
  // Se une cada elemento de la compra
  for (size_t i = 0; i < compra.size(); i++) {
    // Se agrega el separador entre elementos
    if (i > 0) { detalle += ";"; }
    detalle += compra[i].first + "," + std::to_string(compra[i].second);
  }
  // Se envía BUY con la cantidad de elementos y el detalle, y se guarda el texto completo de la isla
  respuesta = this->consultar("40/" + std::to_string(compra.size()) + "/" + detalle);
  // Éxito solo si la isla respondió 01
  return esAck(respuesta);
}

void EnlaceMancomunado::finalizar() {
  // Mensaje de finalización para la isla remota
  Mensaje fin;
  // Se limpia el mensaje para no enviar basura
  std::memset(&fin, 0, sizeof(Mensaje));
  // Se declara la operación "FIN"
  fin.idCliente = FIN;
  // Se indica el comando
  fin.comando = Comando::Fin;
  // Se envía a la isla remota
  this->colaHaciaRemoto.encolar(fin, 1);
  // Registro de bitácora: encolamiento de operación final
  this->bitacora.registrarAccion("Intermediario", "encolamiento de operación final mancomunada");
}
