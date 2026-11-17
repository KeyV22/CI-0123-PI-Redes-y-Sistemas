/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Intermediario2.cc: Implementación del intermediario de la isla remota (protocolo mancomunado)
  *
  * (Fedora version)
  *
 **/

#include "Intermediario2.h"

Intermediario2::Intermediario2(ColaMensajes &colaDesdeIntermediario, ColaMensajes &colaHaciaIntermediario, Bitacora &bitacora) :
    colaDesdeIntermediario(colaDesdeIntermediario),
    colaHaciaIntermediario(colaHaciaIntermediario),
    bitacora(bitacora),
    pid(-1) {}

void Intermediario2::iniciar() {
  // Crea el proceso hijo que ejecutará la isla remota
  this->pid = fork();
  // Si no se pudo crear el proceso se informa el error
  if (this->pid == -1) {
    std::cerr << "No se pudo crear el proceso del Intermediario 2" << std::endl;
  // Código que solo ejecuta el proceso hijo
  } else if (this->pid == 0) {
    try {
      // Carga el catálogo dentro del proceso hijo
      this->cargarCatalogo();
      // Ejecuta el ciclo principal
      this->ejecutar();
    // Si falla una cola se informa y se termina el proceso con error
    } catch (const std::exception &excepcion) {
      std::cerr << "Intermediario 2: " << excepcion.what() << std::endl;
      _exit(1);
    }
    // Termina el proceso hijo sin ejecutar destructores que afecten recursos del padre
    _exit(0);
  }
}

void Intermediario2::esperar() {
  // Si el proceso fue creado, espera a que termine para no dejarlo como zombi
  if (this->pid > 0) {
    // Espera la finalización del proceso hijo
    waitpid(this->pid, nullptr, 0);
    // Indica que ya no hay un proceso activo
    this->pid = -1;
  }
}

void Intermediario2::cargarCatalogo() {
  // Categoría ropa
  this->catalogo.push_back({"ropa", {{"camisa", 1250, 10}, {"pantalon", 2500, 6}}});
  // Categoría calzado
  this->catalogo.push_back({"calzado", {{"zapatos", 3000, 4}, {"sandalias", 1550, 8}}});
}

void Intermediario2::ejecutar() {
  // Registra el inicio de la isla remota
  this->bitacora.registrarAccion("Intermediario 2", "inicialización");
  // Ciclo que atiende solicitudes hasta recibir FIN
  while (true) {
    // Espera una solicitud del intermediario local
    Mensaje solicitud = this->colaDesdeIntermediario.desencolar(1);
    // Si es la señal de fin se sale del ciclo
    if (solicitud.comando == Comando::Fin) {
      break;
    }
    // Procesa el texto del protocolo mancomunado
    std::string texto = this->procesar(solicitud.contenido);
    // Prepara la respuesta limpia
    Mensaje respuesta;
    std::memset(&respuesta, 0, sizeof(Mensaje));
    // Conserva el identificador del cliente de origen
    respuesta.idCliente = solicitud.idCliente;
    // Marca la respuesta como mensaje mancomunado
    respuesta.comando = Comando::Mancomunado;
    // Copia el texto con límite de tamaño
    std::strncpy(respuesta.contenido, texto.c_str(), TAMANIO_MAXIMO - 1);
    // Registra la operación
    this->bitacora.registrarAccion("Intermediario 2", "respuesta mancomunada " + texto.substr(0, 2));
    // Envía la respuesta al intermediario local
    this->colaHaciaIntermediario.encolar(respuesta, 1);
  }
  // Registra la finalización
  this->bitacora.registrarAccion("Intermediario 2", "finalización");
}

std::string Intermediario2::procesar(const std::string &texto) {
  // Separa los campos por /
  std::vector<std::string> partes = separarConVacios(texto, '/');
  // Un tipo que no cumple el formato es un mensaje mal formado
  if (!std::regex_match(partes[0], REGEX_TIPO)) {
    return error(100);
  }
  // Se decide según el tipo de mensaje
  if (partes[0] == "10")
    return atenderCategorias(partes);
  else if (partes[0] == "20")
    return atenderProductos(partes);
  else if (partes[0] == "50")
    return atenderReserva(partes);
  else if (partes[0] == "40")
    return atenderCompra(partes);
  else
    // Cualquier otro tipo no está soportado por la isla
    return error(101);
}

std::string Intermediario2::atenderCategorias(const std::vector<std::string> &partes) {
  // 10 no lleva campos
  if (partes.size() != 1) {
    return error(100);
  }
  // Une los nombres de las categorías con coma
  std::string respuesta = "01/";
  for (size_t i = 0; i < this->catalogo.size(); i++) {
    // Agrega la coma entre categorías
    if (i > 0) { respuesta += ","; }
    respuesta += this->catalogo[i].nombre;
  }
  return respuesta;
}

std::string Intermediario2::atenderProductos(const std::vector<std::string> &partes) {
  // 20 lleva exactamente un campo
  if (partes.size() != 2) {
    return error(100);
  }
  // Valida el formato de la categoría
  if (!std::regex_match(partes[1], REGEX_CATEGORIA)) {
    return error(102);
  }
  // La isla convierte la categoría a minúsculas antes de buscar
  std::string categoria = aMinusculas(partes[1]);
  // Recorre el catálogo buscando la categoría
  for (const CategoriaRemota &actual : this->catalogo) {
    if (actual.nombre == categoria) {
      // Construye 01/cantidad/producto,precio,stock;...
      std::string respuesta = "01/" + std::to_string(actual.productos.size()) + "/";
      for (size_t i = 0; i < actual.productos.size(); i++) {
        // Agrega el separador entre productos
        if (i > 0) { respuesta += ";"; }
        respuesta += actual.productos[i].nombre + "," + formatearPrecio(actual.productos[i].centavos) + "," + std::to_string(actual.productos[i].existencia);
      }
      return respuesta;
    }
  }
  // La categoría no existe en la isla
  return "03/" + categoria;
}

std::string Intermediario2::atenderReserva(const std::vector<std::string> &partes) {
  // 50 lleva exactamente dos campos
  if (partes.size() != 3) {
    return error(100);
  }
  // Valida el formato de los campos
  if (!std::regex_match(partes[1], REGEX_PRODUCTO) || !std::regex_match(partes[2], REGEX_CANTIDAD)) {
    return error(102);
  }
  // La isla convierte el producto a minúsculas antes de buscar
  std::string nombre = aMinusculas(partes[1]);
  // Cantidad solicitada
  int cantidad = std::stoi(partes[2]);
  // Una reserva de cero unidades no tiene sentido
  if (cantidad == 0) {
    return error(102);
  }
  // Busca el producto
  ProductoRemoto *producto = this->buscarProducto(nombre);
  // Si no existe se responde NOT_FOUND
  if (producto == nullptr) {
    return "03/" + nombre;
  }
  // Si no hay existencia suficiente se responde stock insuficiente
  if (cantidad > producto->existencia) {
    return error(103);
  }
  // Guarda la reserva con su vencimiento, reemplazando una anterior del mismo producto
  this->reservas[nombre] = {cantidad, time(nullptr) + VIGENCIA_RESERVA};
  return "01";
}

std::string Intermediario2::atenderCompra(const std::vector<std::string> &partes) {
  // Productos y cantidades en el orden recibido
  Pedido pedido;
  // Cantidad total pedida por producto
  Totales totales;
  // Interpreta y valida el formato del mensaje
  std::string problema = this->interpretarPedido(partes, pedido, totales);
  // Si el formato es inválido se responde el error
  if (!problema.empty()) {
    return problema;
  }
  // Verifica existencia de productos, reservas vigentes y stock, sin modificar nada
  problema = this->verificarPedido(totales);
  // Si algo falla se responde el error y no se modifica nada
  if (!problema.empty()) {
    return problema;
  }
  // Se aplica la compra y se retorna la factura
  return this->facturarPedido(pedido, totales);
}

std::string Intermediario2::interpretarPedido(const std::vector<std::string> &partes, Pedido &pedido, Totales &totales) {
  // 40 lleva exactamente los campos n y la lista de productos
  if (partes.size() != 3) {
    return error(100);
  }
  // Valida el formato de n
  if (!std::regex_match(partes[1], REGEX_CONTEO)) {
    return error(102);
  }
  // Separa la lista de productos
  std::vector<std::string> elementos = separarConVacios(partes[2], ';');
  // n debe coincidir con la cantidad de elementos
  if (static_cast<size_t>(std::stoi(partes[1])) != elementos.size()) {
    return error(100);
  }
  // Valida cada elemento y lo agrega al pedido
  for (const std::string &elemento : elementos) {
    // Valores extraídos del elemento
    std::string nombre;
    int cantidad = 0;
    // Valida el elemento producto,cantidad
    std::string problema = this->interpretarElemento(elemento, nombre, cantidad);
    // Si el elemento es inválido se responde el error
    if (!problema.empty()) {
      return problema;
    }
    // Conserva el orden recibido para la factura
    pedido.push_back({nombre, cantidad});
    // Acumula la cantidad total por si el producto se repite
    totales[nombre] += cantidad;
  }
  // El formato es válido
  return "";
}

std::string Intermediario2::interpretarElemento(const std::string &elemento, std::string &nombre, int &cantidad) {
  // Cada elemento es producto,cantidad
  std::vector<std::string> campos = separarConVacios(elemento, ',');
  // Debe tener exactamente dos campos
  if (campos.size() != 2) {
    return error(100);
  }
  // Valida el formato del producto y de la cantidad, que además no puede ser cero
  if (!std::regex_match(campos[0], REGEX_PRODUCTO) || !std::regex_match(campos[1], REGEX_CANTIDAD) || std::stoi(campos[1]) == 0) {
    return error(102);
  }
  // Normaliza el nombre del producto a minúsculas
  nombre = aMinusculas(campos[0]);
  // Convierte la cantidad a número
  cantidad = std::stoi(campos[1]);
  // El elemento es válido
  return "";
}

std::string Intermediario2::verificarPedido(const Totales &totales) {
  // Revisa cada producto del pedido
  for (const auto &total : totales) {
    // Busca el producto
    ProductoRemoto *producto = this->buscarProducto(total.first);
    // Si el producto no existe se responde NOT_FOUND
    if (producto == nullptr) {
      return "03/" + total.first;
    }
    // Busca la reserva del producto
    auto reserva = this->reservas.find(total.first);
    // Sin reserva vigente (inexistente o expirada) o con cantidad menor a la pedida
    if (reserva == this->reservas.end() || reserva->second.vence < time(nullptr) || reserva->second.cantidad < total.second) {
      return error(104);
    }
    // Si ya no hay existencia suficiente se responde stock insuficiente
    if (producto->existencia < total.second) {
      return error(103);
    }
  }
  // Todo el pedido es comprable
  return "";
}

std::string Intermediario2::facturarPedido(const Pedido &pedido, const Totales &totales) {
  // Total de la factura en centavos
  long totalCentavos = 0;
  // Detalle de la factura: producto,cantidad,subtotal;...
  std::string detalle;
  // Procesa cada elemento en el orden recibido
  for (size_t i = 0; i < pedido.size(); i++) {
    // Producto comprado, ya verificado en el paso anterior
    ProductoRemoto *producto = this->buscarProducto(pedido[i].first);
    // Descuenta el stock
    producto->existencia -= pedido[i].second;
    // Calcula el subtotal del elemento
    long subtotal = producto->centavos * pedido[i].second;
    // Suma al total de la factura
    totalCentavos += subtotal;
    // Agrega el separador entre productos
    if (i > 0) { detalle += ";"; }
    // Agrega el elemento al detalle
    detalle += pedido[i].first + "," + std::to_string(pedido[i].second) + "," + formatearPrecio(subtotal);
  }
  // Las reservas usadas se eliminan
  for (const auto &total : totales) {
    this->reservas.erase(total.first);
  }
  // Construye 01/total/n/producto,cantidad,subtotal;...
  return "01/" + formatearPrecio(totalCentavos) + "/" + std::to_string(pedido.size()) + "/" + detalle;
}

Intermediario2::ProductoRemoto *Intermediario2::buscarProducto(const std::string &nombre) {
  // Recorre todas las categorías
  for (CategoriaRemota &categoria : this->catalogo) {
    // Recorre los productos de la categoría
    for (ProductoRemoto &producto : categoria.productos) {
      if (producto.nombre == nombre) {
        return &producto;
      }
    }
  }
  // No se encontró
  return nullptr;
}

std::string Intermediario2::error(int codigo) {
  // Construye 02/código
  return "02/" + std::to_string(codigo);
}
