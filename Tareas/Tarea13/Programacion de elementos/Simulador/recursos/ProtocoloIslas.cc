#include "ProtocoloIslas.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <regex>

// Expresiones regulares del protocolo
static const char *REGEX_TIPO     = "^[0-9]{2}$";
static const char *REGEX_ID_MSG   = "^[0-9]{4}$";
static const char *REGEX_ID_ISLA  = "^INT_[0-9]{2}$";
static const char *REGEX_IP       = "^([0-9]{1,3}\\.){3}[0-9]{1,3}$";
static const char *REGEX_PUERTO   = "^[0-9]{1,5}$";
static const char *REGEX_NOMBRE   = "^[a-zA-Z0-9_\\-]{1,50}$";
static const char *REGEX_CANTIDAD = "^[0-9]{1,5}$";
static const char *REGEX_COUNT    = "^[0-9]{1,3}$";
static const char *REGEX_PRECIO   = "^[0-9]{1,6}(\\.[0-9]{1,2})?$";

ProtocoloIslas::ProtocoloIslas(SistemaArchivos &sistemaArchivos, Bitacora &bitacora) :
    sistemaArchivos(sistemaArchivos),
    bitacora(bitacora),
    vigenciaSegundos(60) {}

void ProtocoloIslas::fijarVigenciaReserva(int segundos) {
  this->vigenciaSegundos = segundos;
}

std::string ProtocoloIslas::error(const std::string &codigo) { return "02/" + codigo; }

std::string ProtocoloIslas::noEncontrado(const std::string &valor) { return "03/" + valor; }

std::string ProtocoloIslas::minusculas(std::string texto) {
  for (char &c : texto) { c = (char)std::tolower((unsigned char)c); }
  return texto;
}

std::string ProtocoloIslas::decimal(double valor) {
  char bufer[64];
  std::snprintf(bufer, sizeof(bufer), "%.2f", valor);
  return bufer;
}

bool ProtocoloIslas::valido(const std::string &valor, const char *expresion) {
  return std::regex_match(valor, std::regex(expresion));
}

// Busca un producto en todas las bodegas; la comparación no distingue mayúsculas
bool ProtocoloIslas::buscarProducto(const std::string &nombre, ProductoTexto &encontrado) {
  for (const std::string &bodega : this->sistemaArchivos.listar_bodegas()) {
    for (const ProductoTexto &p : this->sistemaArchivos.listar_productos(bodega)) {
      if (minusculas(p.producto) == minusculas(nombre)) {
        encontrado = p;
        return true;
      }
    }
  }
  return false;
}

// Elimina las reservas cuya vigencia ya terminó
void ProtocoloIslas::descartarVencidas() {
  auto ahora = std::chrono::steady_clock::now();
  std::vector<Reserva> vigentes;
  for (const Reserva &r : this->reservas) {
    if (r.vence > ahora) { vigentes.push_back(r); }
    else { this->bitacora.registrarAccion("Protocolo", "reserva vencida: " + r.producto); }
  }
  this->reservas = vigentes;
}

// Total reservado de un producto
int ProtocoloIslas::reservado(const std::string &producto) {
  int total = 0;
  for (const Reserva &r : this->reservas) {
    if (r.producto == minusculas(producto)) { total += r.cantidad; }
  }
  return total;
}

// Gasta la reserva de un producto (la más antigua primero)
void ProtocoloIslas::consumirReserva(const std::string &producto, int cantidad) {
  for (Reserva &r : this->reservas) {
    if (cantidad == 0) { break; }
    if (r.producto != minusculas(producto)) { continue; }
    int usado = std::min(r.cantidad, cantidad);
    r.cantidad -= usado;
    cantidad -= usado;
  }
  std::vector<Reserva> restantes;
  for (const Reserva &r : this->reservas) {
    if (r.cantidad > 0) { restantes.push_back(r); }
  }
  this->reservas = restantes;
}

int ProtocoloIslas::existencia(const ProductoTexto &p) {
  try { return std::stoi(p.cantidad); } catch (...) { return 0; }
}

// Cambia la cantidad de un producto conservando categoría y precio
void ProtocoloIslas::cambiarExistencia(const ProductoTexto &p, int nueva) {
  this->sistemaArchivos.extraer_producto(p.bodega, p.producto);
  this->sistemaArchivos.insertar_producto(p.bodega, p.categoria, p.producto, std::to_string(nueva), p.precio);
}

std::string ProtocoloIslas::procesar(const std::string &mensaje) {
  this->descartarVencidas();
  this->bitacora.registrarAccion("Protocolo", "mensaje recibido: " + mensaje);
  std::vector<std::string> campos = separarCampos(mensaje, '/');
  // Sin tipo o con tipo mal formado: mensaje mal formado
  if (campos.empty() || !valido(campos[0], REGEX_TIPO)) { return error("100"); }
  const std::string &tipo = campos[0];
  if (tipo == "60") return atenderAnnounce(campos);
  if (tipo == "61") return atenderAnnounceBye(campos);
  if (tipo == "10") return atenderCategorias(campos);
  if (tipo == "20") return atenderProductos(campos);
  if (tipo == "50") return atenderReserva(campos);
  if (tipo == "40") return atenderCompra(campos);
  // Cualquier otro tipo no se soporta como solicitud
  return error("101");
}

// 60/ID_MSG/id_isla/ip/puerto (UDP: nunca se responde)
std::string ProtocoloIslas::atenderAnnounce(const std::vector<std::string> &c) {
  if (c.size() != 5 || !valido(c[1], REGEX_ID_MSG) || !valido(c[2], REGEX_ID_ISLA)
      || !valido(c[3], REGEX_IP) || !valido(c[4], REGEX_PUERTO)) {
    this->bitacora.registrarAccion("Protocolo", "ANNOUNCE descartado: formato inválido");
    return "";
  }
  if (!this->idsVistos.insert(c[1]).second) {
    this->bitacora.registrarAccion("Protocolo", "ANNOUNCE descartado: ID_MSG repetido " + c[1]);
    return "";
  }
  this->islas[c[2]] = Isla{c[3], c[4]};
  this->bitacora.registrarAccion("Protocolo", "isla registrada: " + c[2] + " " + c[3] + ":" + c[4]);
  return "";
}

// 61/id_isla (UDP: nunca se responde)
std::string ProtocoloIslas::atenderAnnounceBye(const std::vector<std::string> &c) {
  if (c.size() != 2 || !valido(c[1], REGEX_ID_ISLA)) {
    this->bitacora.registrarAccion("Protocolo", "ANNOUNCE_BYE descartado: formato inválido");
    return "";
  }
  this->islas.erase(c[1]);
  this->bitacora.registrarAccion("Protocolo", "isla eliminada: " + c[1]);
  return "";
}

// 10 -> 01/categoria1,categoria2,...
std::string ProtocoloIslas::atenderCategorias(const std::vector<std::string> &c) {
  if (c.size() != 1) { return error("100"); }
  std::vector<std::string> categorias;
  for (const std::string &bodega : this->sistemaArchivos.listar_bodegas()) {
    for (const ProductoTexto &p : this->sistemaArchivos.listar_productos(bodega)) {
      if (!valido(p.categoria, REGEX_NOMBRE)) { continue; }
      if (std::find(categorias.begin(), categorias.end(), p.categoria) == categorias.end()) {
        categorias.push_back(p.categoria);
      }
    }
  }
  std::string lista;
  for (size_t i = 0; i < categorias.size(); i++) {
    if (i > 0) { lista += ","; }
    lista += categorias[i];
  }
  return "01/" + lista;
}

// 20/categoria -> 01/cantidad/producto,precio,stock;...
std::string ProtocoloIslas::atenderProductos(const std::vector<std::string> &c) {
  if (c.size() != 2) { return error("100"); }
  if (!valido(c[1], REGEX_NOMBRE)) { return error("102"); }
  // La isla pasa la categoría a minúsculas antes de buscar
  std::string categoria = minusculas(c[1]);
  int cantidad = 0;
  std::string lista;
  for (const std::string &bodega : this->sistemaArchivos.listar_bodegas()) {
    for (const ProductoTexto &p : this->sistemaArchivos.listar_productos(bodega)) {
      if (minusculas(p.categoria) != categoria) { continue; }
      if (!valido(p.producto, REGEX_NOMBRE) || !valido(p.precio, REGEX_PRECIO)) { continue; }
      // El stock que se informa descuenta lo que ya está reservado
      int stock = existencia(p) - reservado(p.producto);
      if (cantidad > 0) { lista += ";"; }
      lista += p.producto + "," + decimal(std::stod(p.precio)) + "," + std::to_string(stock < 0 ? 0 : stock);
      cantidad++;
    }
  }
  if (cantidad == 0) { return noEncontrado(c[1]); }
  return "01/" + std::to_string(cantidad) + "/" + lista;
}

// 50/producto/cantidad -> 01 (reserva por la vigencia configurada)
std::string ProtocoloIslas::atenderReserva(const std::vector<std::string> &c) {
  if (c.size() != 3) { return error("100"); }
  if (!valido(c[1], REGEX_NOMBRE) || !valido(c[2], REGEX_CANTIDAD) || std::stoi(c[2]) == 0) {
    return error("102");
  }
  ProductoTexto producto;
  if (!buscarProducto(c[1], producto)) { return noEncontrado(c[1]); }
  int cantidad = std::stoi(c[2]);
  if (cantidad > existencia(producto) - reservado(producto.producto)) { return error("103"); }
  Reserva r;
  r.producto = minusculas(producto.producto);
  r.cantidad = cantidad;
  r.vence = std::chrono::steady_clock::now() + std::chrono::seconds(this->vigenciaSegundos);
  this->reservas.push_back(r);
  this->bitacora.registrarAccion("Protocolo", "reserva: " + r.producto + " x" + c[2]);
  return "01";
}

// 40/n/producto,cantidad;... -> 01/total/n/producto,cantidad,subtotal;...
std::string ProtocoloIslas::atenderCompra(const std::vector<std::string> &c) {
  if (c.size() != 3 || !valido(c[1], REGEX_COUNT)) { return error("100"); }
  std::vector<std::string> items = separarCampos(c[2], ';');
  if (items.empty() || (int)items.size() != std::stoi(c[1])) { return error("100"); }

  // Fase 1: valida todo sin modificar nada, para que la compra sea completa o no se haga
  struct Linea { ProductoTexto producto; int cantidad; };
  std::vector<Linea> lineas;
  std::map<std::string, int> reservaLibre;
  std::map<std::string, int> stockLibre;
  for (const std::string &item : items) {
    std::vector<std::string> par = separarCampos(item, ',');
    if (par.size() != 2) { return error("100"); }
    if (!valido(par[0], REGEX_NOMBRE) || !valido(par[1], REGEX_CANTIDAD) || std::stoi(par[1]) == 0) {
      return error("102");
    }
    ProductoTexto producto;
    if (!buscarProducto(par[0], producto)) { return noEncontrado(par[0]); }
    std::string clave = minusculas(producto.producto);
    if (!reservaLibre.count(clave)) {
      reservaLibre[clave] = reservado(clave);
      stockLibre[clave] = existencia(producto);
    }
    int cantidad = std::stoi(par[1]);
    if (reservaLibre[clave] < cantidad) { return error("104"); }
    if (stockLibre[clave] < cantidad) { return error("103"); }
    reservaLibre[clave] -= cantidad;
    stockLibre[clave] -= cantidad;
    lineas.push_back(Linea{producto, cantidad});
  }

  // Fase 2: gasta las reservas, descuenta las existencias y arma la factura
  double total = 0;
  std::string detalle;
  for (size_t i = 0; i < lineas.size(); i++) {
    ProductoTexto actual;
    buscarProducto(lineas[i].producto.producto, actual);
    consumirReserva(actual.producto, lineas[i].cantidad);
    cambiarExistencia(actual, existencia(actual) - lineas[i].cantidad);
    double subtotal = std::stod(actual.precio) * lineas[i].cantidad;
    total += subtotal;
    if (i > 0) { detalle += ";"; }
    detalle += actual.producto + "," + std::to_string(lineas[i].cantidad) + "," + decimal(subtotal);
  }
  this->bitacora.registrarAccion("Protocolo", "compra realizada, total " + decimal(total));
  return "01/" + decimal(total) + "/" + std::to_string(lineas.size()) + "/" + detalle;
}