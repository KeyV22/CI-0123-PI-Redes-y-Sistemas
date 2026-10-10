#include "Servidor.h"

Servidor::Servidor(Buzon &buzonHaciaIntermediario, Buzon &buzonDesdeIntermediario, SistemaArchivos &sistemaArchivos, Bitacora &bitacora) :
    buzonHaciaIntermediario(buzonHaciaIntermediario),
    buzonDesdeIntermediario(buzonDesdeIntermediario),
    sistemaArchivos(sistemaArchivos),
    bitacora(bitacora),
    protocolo(sistemaArchivos, bitacora),
    estado(Estado::Esperando) {}

Servidor::~Servidor() {}

void Servidor::iniciar() {
  // Registro de bitácora: inicialización
  this->bitacora.registrarAccion("Servidor", "inicialización");
  // Inicializa y ejecuta el hilo del servidor
  this->hilo = std::thread(&Servidor::ejecutar, this);
}

void Servidor::fijarVigenciaReserva(int segundos) {
  this->protocolo.fijarVigenciaReserva(segundos);
}

void Servidor::detener() {
  // Se crea un mensaje para finalizar el programa
  Mensaje fin;
  // Se declara la operación "FIN"
  fin.idCliente = FIN;
  // Se indica el comando
  fin.comando = Comando::Fin;
  // Se copia el contenido indicando el final
  strcpy(fin.contenido, "TERMINAR");
  // Se ingresa el mensaje en el buzón
  this->buzonDesdeIntermediario.encolar(fin);
  // Registro de bitácora: encolamiento de operación final
  this->bitacora.registrarAccion("Servidor", "encolamiento de operación final");
  // Se cambia el estado del servidor
  this->estado = Estado::Detenido;
  // Se finaliza el hilo de ejecución
  if (this->hilo.joinable()) {
    this->hilo.join();
  }
}

Mensaje Servidor::mensajeInvalido(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  // Se asigna la identificación del cliente
  respuesta.idCliente = solicitud.idCliente;
  // Se agrega el comando de resultado del mensaje
  respuesta.comando = Comando::ErrorFormatoInvalido;
  // Se copia el contenido del mensaje
  strcpy(respuesta.contenido, "Comando no soportado");
  // Se retorna la estructura creada
  return respuesta;
}

bool Servidor::buscarEnTodasLasBodegas(const std::string &nombreProducto, ProductoTexto &productoEncontrado) {
  // Recorre todas las bodegas existentes
  for (const std::string &bodega : this->sistemaArchivos.listar_bodegas()) {
    // Recorre los productos de cada bodega
    for (const ProductoTexto &producto : this->sistemaArchivos.listar_productos(bodega)) {
      // Compara el producto consultado
      if (producto.producto == nombreProducto) {
        // Si coincide, guarda el producto encontrado y retorna éxito
        productoEncontrado = producto;
        return true;
      }
    }
  }
  // No se encontró el producto en ninguna bodega
  return false;
}

Mensaje Servidor::atenderBuscarProducto(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  // El contenido trae únicamente el nombre del producto buscado
  ProductoTexto producto;
  if (this->buscarEnTodasLasBodegas(solicitud.contenido, producto)) {
    // Se arma la respuesta con los datos encontrados
    respuesta.comando = Comando::Disponible;
    std::string contenido = producto.bodega + ":" + producto.categoria + ":" + producto.producto + ":" + producto.cantidad + ":" + producto.precio;
    strcpy(respuesta.contenido, contenido.c_str());
  } else {
    respuesta.comando = Comando::ErrorNoDisponible;
    // Se usa el identificador del producto buscado, no un texto descriptivo
    strcpy(respuesta.contenido, solicitud.contenido);
  }
  return respuesta;
}

Mensaje Servidor::atenderCategorias(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  // Vector local para acumular las categorías ya vistas, sin repetir
  std::vector<std::string> categorias;
  // Recorre todas las bodegas y todos sus productos
  for (const std::string &bodega : this->sistemaArchivos.listar_bodegas()) {
    for (const ProductoTexto &producto : this->sistemaArchivos.listar_productos(bodega)) {
      // Agrega la categoría solo si todavía no está en el vector
      bool yaExiste = false;
      for (const std::string &categoria : categorias) {
        if (categoria == producto.categoria) { yaExiste = true; break; }
      }
      if (!yaExiste) { categorias.push_back(producto.categoria); }
    }
  }
  // Arma la respuesta con la lista de categorías encontradas
  if (categorias.empty()) {
    respuesta.comando = Comando::Ok;
    strcpy(respuesta.contenido, "NINGUNA");
  } else {
    std::string contenido;
    for (size_t i = 0; i < categorias.size(); i++) {
      if (i > 0) { contenido += ":"; }
      contenido += categorias[i];
    }
    respuesta.comando = Comando::Ok;
    strcpy(respuesta.contenido, contenido.c_str());
  }
  return respuesta;
}

Mensaje Servidor::atenderProductos(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  // Separa la categoría y la bodega (opcional) del contenido recibido
  std::vector<std::string> campos = separarCampos(solicitud.contenido, ':');
  std::string idCategoria = campos.size() > 0 ? campos[0] : "";
  std::string idBodega = campos.size() > 1 ? campos[1] : "";
  // Si se indicó bodega, verifica que exista
  if (!idBodega.empty()) {
    bool bodegaExiste = false;
    for (const std::string &bodega : this->sistemaArchivos.listar_bodegas()) {
      if (bodega == idBodega) { bodegaExiste = true; break; }
    }
    if (!bodegaExiste) {
      respuesta.comando = Comando::ErrorBodegaInexistente;
      strcpy(respuesta.contenido, idBodega.c_str());
      return respuesta;
    }
  }
  // Acumula los productos que coincidan con la categoría solicitada
  std::vector<std::string> bodegasARevisar;
  if (!idBodega.empty()) {
    bodegasARevisar.push_back(idBodega);
  } else {
    bodegasARevisar = this->sistemaArchivos.listar_bodegas();
  }
  std::string contenido;
  for (const std::string &bodega : bodegasARevisar) {
    for (const ProductoTexto &producto : this->sistemaArchivos.listar_productos(bodega)) {
      if (producto.categoria == idCategoria) {
        if (!contenido.empty()) { contenido += ":"; }
        contenido += producto.producto;
      }
    }
  }
  if (contenido.empty()) {
    respuesta.comando = Comando::ErrorCategoriaInexistente;
    strcpy(respuesta.contenido, idCategoria.c_str());
  } else {
    respuesta.comando = Comando::Ok;
    strcpy(respuesta.contenido, contenido.c_str());
  }
  return respuesta;
}

Mensaje Servidor::atenderExistencia(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  // Separa el producto y la bodega (opcional) del contenido recibido
  std::vector<std::string> campos = separarCampos(solicitud.contenido, ':');
  std::string idProducto = campos.size() > 0 ? campos[0] : "";
  std::string idBodega = campos.size() > 1 ? campos[1] : "";
  if (!idBodega.empty()) {
    // Busca solo en la bodega indicada
    for (const ProductoTexto &producto : this->sistemaArchivos.listar_productos(idBodega)) {
      if (producto.producto == idProducto) {
        respuesta.comando = Comando::Si;
        strcpy(respuesta.contenido, producto.cantidad.c_str());
        return respuesta;
      }
    }
    respuesta.comando = Comando::No;
    strcpy(respuesta.contenido, "");
    return respuesta;
  }
  // Sin bodega indicada: busca en todas
  ProductoTexto producto;
  if (this->buscarEnTodasLasBodegas(idProducto, producto)) {
    respuesta.comando = Comando::Si;
    strcpy(respuesta.contenido, producto.cantidad.c_str());
  } else {
    respuesta.comando = Comando::No;
    strcpy(respuesta.contenido, "");
  }
  return respuesta;
}

Mensaje Servidor::atenderFiltrar(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  // Separa categoría, criterio y bodega (opcional) del contenido recibido
  std::vector<std::string> campos = separarCampos(solicitud.contenido, ':');
  std::string idCategoria = campos.size() > 0 ? campos[0] : "";
  std::string criterio = campos.size() > 1 ? campos[1] : "";
  std::string idBodega = campos.size() > 2 ? campos[2] : "";
  std::vector<std::string> bodegasARevisar;
  if (!idBodega.empty()) {
    bodegasARevisar.push_back(idBodega);
  } else {
    bodegasARevisar = this->sistemaArchivos.listar_bodegas();
  }
  std::string contenido;
  for (const std::string &bodega : bodegasARevisar) {
    for (const ProductoTexto &producto : this->sistemaArchivos.listar_productos(bodega)) {
      // Coincide si la categoría calza y el nombre del producto contiene el criterio
      if (producto.categoria == idCategoria && producto.producto.find(criterio) != std::string::npos) {
        if (!contenido.empty()) { contenido += ":"; }
        contenido += producto.producto;
      }
    }
  }
  if (contenido.empty()) {
    respuesta.comando = Comando::SinCoincidencia;
    strcpy(respuesta.contenido, "");
  } else {
    respuesta.comando = Comando::Coincidencia;
    strcpy(respuesta.contenido, contenido.c_str());
  }
  return respuesta;
}

Mensaje Servidor::atenderCrearProducto(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  // Separa producto, categoría, bodega, cantidad y precio del contenido recibido
  std::vector<std::string> campos = separarCampos(solicitud.contenido, ':');
  if (campos.size() < 5) {
    respuesta.comando = Comando::ErrorFormatoInvalido;
    strcpy(respuesta.contenido, "CREAR_PRODUCTO");
    return respuesta;
  }
  std::string idProducto = campos[0], idCategoria = campos[1], idBodega = campos[2], cantidad = campos[3], precio = campos[4];
  // Verifica que la bodega exista antes de intentar insertar
  bool bodegaExiste = false;
  for (const std::string &bodega : this->sistemaArchivos.listar_bodegas()) {
    if (bodega == idBodega) { bodegaExiste = true; break; }
  }
  if (!bodegaExiste) {
    respuesta.comando = Comando::ErrorBodegaInexistente;
    strcpy(respuesta.contenido, idBodega.c_str());
    return respuesta;
  }
  // Intenta insertar el producto
  if (this->sistemaArchivos.insertar_producto(idBodega, idCategoria, idProducto, cantidad, precio)) {
    respuesta.comando = Comando::Ok;
    std::string contenido = idProducto + ":" + idBodega;
    strcpy(respuesta.contenido, contenido.c_str());
  } else {
    respuesta.comando = Comando::ErrorProductoExistente;
    strcpy(respuesta.contenido, idProducto.c_str());
  }
  return respuesta;
}

Mensaje Servidor::atenderActualizarProducto(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  // Separa producto, bodega, cantidad y precio del contenido recibido
  std::vector<std::string> campos = separarCampos(solicitud.contenido, ':');
  if (campos.size() < 4) {
    respuesta.comando = Comando::ErrorFormatoInvalido;
    strcpy(respuesta.contenido, "ACTUALIZAR_PRODUCTO");
    return respuesta;
  }
  std::string idProducto = campos[0], idBodega = campos[1], cantidad = campos[2], precio = campos[3];
  // Busca el registro actual para conservar su categoría original
  std::string categoriaActual;
  bool productoExiste = false;
  for (const ProductoTexto &producto : this->sistemaArchivos.listar_productos(idBodega)) {
    if (producto.producto == idProducto) {
      categoriaActual = producto.categoria;
      productoExiste = true;
      break;
    }
  }
  if (!productoExiste) {
    respuesta.comando = Comando::ErrorNoDisponible;
    strcpy(respuesta.contenido, idProducto.c_str());
    return respuesta;
  }
  // Quita el registro viejo y agrega el nuevo con los datos actualizados
  this->sistemaArchivos.extraer_producto(idBodega, idProducto);
  this->sistemaArchivos.insertar_producto(idBodega, categoriaActual, idProducto, cantidad, precio);
  respuesta.comando = Comando::OkCatalogo;
  std::string contenido = idProducto + ":" + idBodega;
  strcpy(respuesta.contenido, contenido.c_str());
  return respuesta;
}

Mensaje Servidor::atenderEliminarProducto(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  // Separa producto y bodega del contenido recibido
  std::vector<std::string> campos = separarCampos(solicitud.contenido, ':');
  if (campos.size() < 2) {
    respuesta.comando = Comando::ErrorFormatoInvalido;
    strcpy(respuesta.contenido, "ELIMINAR_PRODUCTO");
    return respuesta;
  }
  std::string idProducto = campos[0], idBodega = campos[1];
  if (this->sistemaArchivos.extraer_producto(idBodega, idProducto)) {
    respuesta.comando = Comando::Ok;
    std::string contenido = idProducto + ":" + idBodega;
    strcpy(respuesta.contenido, contenido.c_str());
  } else {
    respuesta.comando = Comando::ErrorNoDisponible;
    strcpy(respuesta.contenido, idProducto.c_str());
  }
  return respuesta;
}

Mensaje Servidor::atenderProtocolo(const Mensaje &solicitud) {
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  respuesta.comando = Comando::RespuestaProtocolo;
  // El contenido de la solicitud es el texto del mensaje del protocolo
  std::string texto = this->protocolo.procesar(solicitud.contenido);
  // Se copia la respuesta sin desbordar el búfer del mensaje
  std::strncpy(respuesta.contenido, texto.c_str(), TAMANIO_MAXIMO - 1);
  respuesta.contenido[TAMANIO_MAXIMO - 1] = '\0';
  return respuesta;
}

Mensaje Servidor::procesarSolicitud(const Mensaje &solicitud) {
  // Se evalúa el comando de la solicitud y se delega al método correspondiente
  if (solicitud.comando == Comando::BuscarProducto)
    return atenderBuscarProducto(solicitud);
  else if (solicitud.comando == Comando::Categorias)
    return atenderCategorias(solicitud);
  else if (solicitud.comando == Comando::Productos)
    return atenderProductos(solicitud);
  else if (solicitud.comando == Comando::Existencia)
    return atenderExistencia(solicitud);
  else if (solicitud.comando == Comando::Filtrar)
    return atenderFiltrar(solicitud);
  else if (solicitud.comando == Comando::CrearProducto)
    return atenderCrearProducto(solicitud);
  else if (solicitud.comando == Comando::ActualizarProducto)
    return atenderActualizarProducto(solicitud);
  else if (solicitud.comando == Comando::EliminarProducto)
    return atenderEliminarProducto(solicitud);
  else if (solicitud.comando == Comando::Protocolo)
    return atenderProtocolo(solicitud);
  else
    return mensajeInvalido(solicitud);
}

void Servidor::ejecutar() {
  // Se ejecuta mientras el estado del servidor no sea detenido
  while (this->estado != Estado::Detenido) {
    // Se declara que el servidor está esperando
    this->estado = Estado::Esperando;
    // Se recibe la solicitud del intermediario
    Mensaje solicitud = this->buzonDesdeIntermediario.desencolar();
    // Registro de bitácora: desencolamiento de mensaje
    this->bitacora.registrarAccion("Servidor", "desencolamiento de mensaje");
    // Se declara que el servidor está procesando la solicitud del intermediario
    this->estado = Estado::Procesando;
    // Si se indica la operación "FIN", finaliza la operación actual
    if (solicitud.idCliente == FIN) { break; }
    // Se crea la respuesta para el intermediario
    Mensaje respuesta = procesarSolicitud(solicitud);
    // Registro de bitácora: proceso de solicitud
    this->bitacora.registrarAccion("Servidor", "proceso de solicitud");
    // Se encola la respuesta creada para el intermediario
    this->buzonHaciaIntermediario.encolar(respuesta);
    // Registro de bitácora: encolamiento de mensaje
    this->bitacora.registrarAccion("Servidor", "encolamiento de mensaje");
  }
}
