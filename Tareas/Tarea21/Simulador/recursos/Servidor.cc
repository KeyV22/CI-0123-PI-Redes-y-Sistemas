/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Servidor.cc: Implementación de la clase Servidor (proceso independiente)
  *
  * (Fedora version)
  *
 **/

#include "Servidor.h"

Servidor::Servidor(ColaMensajes &colaDesdeIntermediario, ColaMensajes &colaHaciaIntermediario, long idServidor, const std::string &rutaBodegas, Bitacora &bitacora, std::function<void(SistemaArchivos &)> preparar) :
    colaDesdeIntermediario(colaDesdeIntermediario),
    colaHaciaIntermediario(colaHaciaIntermediario),
    idServidor(idServidor),
    rutaBodegas(rutaBodegas),
    bitacora(bitacora),
    preparar(preparar),
    pid(-1) {}

void Servidor::iniciar() {
  // Crea el proceso hijo que ejecutará el servidor
  this->pid = fork();
  // Si no se pudo crear el proceso se informa el error
  if (this->pid == -1) {
    std::cerr << "No se pudo crear el proceso del " << this->nombre() << std::endl;
  // Código que solo ejecuta el proceso hijo
  } else if (this->pid == 0) {
    // Ejecuta el ciclo principal del servidor
    try {
      // Ejecuta el ciclo del proceso
      this->ejecutar();
    // Si falla una cola se informa y se termina el proceso con error
    } catch (const std::exception &excepcion) {
      std::cerr << this->nombre() << ": " << excepcion.what() << std::endl;
      _exit(1);
    }
    // Termina el proceso hijo sin ejecutar destructores que afecten recursos del padre
    _exit(0);
  }
}

void Servidor::esperar() {
  // Si el proceso fue creado, espera a que termine para no dejarlo como zombi
  if (this->pid > 0) {
    // Espera la finalización del proceso hijo
    waitpid(this->pid, nullptr, 0);
    // Indica que ya no hay un proceso activo
    this->pid = -1;
  }
}

std::string Servidor::nombre() {
  // Nombre con el que el servidor se identifica en la bitácora
  return "Servidor " + std::to_string(this->idServidor);
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
  else
    return mensajeInvalido(solicitud);
}

Mensaje Servidor::mensajeInvalido(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  // Se asigna la identificación del cliente
  respuesta.idCliente = solicitud.idCliente;
  // Se agrega el comando de resultado del mensaje
  respuesta.comando = Comando::ErrorFormatoInvalido;
  // Se copia el contenido del mensaje
  copiarTexto(respuesta.contenido, "Comando no soportado");
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

Mensaje Servidor::filtrar(Mensaje &respuesta, const std::vector<std::string> &bodegasARevisar, const std::string &idCategoria, const std::string &criterio) {
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
    copiarTexto(respuesta.contenido, "");
  } else {
    respuesta.comando = Comando::Coincidencia;
    // Si la lista no cabe completa se registra en la bitácora
    if (!copiarLista(respuesta.contenido, contenido, ':')) {
      this->bitacora.registrarAccion(this->nombre(), "lista truncada al tamaño máximo del mensaje");
    }
  }
  return respuesta;
}

bool Servidor::bodegaExiste(const std::string &idBodega) {
  // Recorre las bodegas del sistema de archivos
  for (const std::string &bodega : this->sistemaArchivos.listar_bodegas()) {
    // Si el nombre coincide, la bodega existe
    if (bodega == idBodega) { return true; }
  }
  // No se encontró la bodega
  return false;
}

bool Servidor::productoEnBodega(const std::string &idBodega, const std::string &idProducto) {
  // Recorre los productos de la bodega indicada
  for (const ProductoTexto &producto : this->sistemaArchivos.listar_productos(idBodega)) {
    // Si el nombre coincide, el producto ya existe
    if (producto.producto == idProducto) { return true; }
  }
  // No se encontró el producto en la bodega
  return false;
}

Mensaje Servidor::actualizarProducto(Mensaje &respuesta, const std::string &idProducto, const std::string &idBodega, const std::string &cantidad, const std::string &precio) {
  // Registro actual del producto, del que se conserva la categoría original
  ProductoTexto anterior;
  // Indica si el producto fue encontrado en la bodega
  bool productoExiste = false;
  // Busca el registro actual del producto
  for (const ProductoTexto &producto : this->sistemaArchivos.listar_productos(idBodega)) {
    if (producto.producto == idProducto) {
      anterior = producto;
      productoExiste = true;
      break;
    }
  }
  // Si el producto no existe se responde con error
  if (!productoExiste) {
    respuesta.comando = Comando::ErrorNoDisponible;
    copiarTexto(respuesta.contenido, idProducto);
    return respuesta;
  }
  // Quita el registro viejo
  this->sistemaArchivos.extraer_producto(idBodega, idProducto);
  // Agrega el registro nuevo con los datos actualizados; si no cabe en un bloque, se restaura el anterior para no perder el producto
  if (!this->sistemaArchivos.insertar_producto(idBodega, anterior.categoria, idProducto, cantidad, precio)) {
    this->sistemaArchivos.insertar_producto(idBodega, anterior.categoria, anterior.producto, anterior.cantidad, anterior.precio);
    respuesta.comando = Comando::ErrorFormatoInvalido;
    copiarTexto(respuesta.contenido, "ACTUALIZAR_PRODUCTO");
    return respuesta;
  }
  respuesta.comando = Comando::OkCatalogo;
  std::string contenido = idProducto + ":" + idBodega;
  copiarTexto(respuesta.contenido, contenido);
  return respuesta;
}

Mensaje Servidor::imprimirCategorias(Mensaje &respuesta, std::vector<std::string> &categorias) {
  // Arma la respuesta con la lista de categorías encontradas 
  if (categorias.empty()) {
    respuesta.comando = Comando::Ok;
    copiarTexto(respuesta.contenido, "NINGUNA");
  } else {
    std::string contenido;
    for (size_t i = 0; i < categorias.size(); i++) {
      if (i > 0) { contenido += ":"; }
      contenido += categorias[i];
    }
    respuesta.comando = Comando::Ok;
    // Si la lista no cabe completa se registra en la bitácora
    if (!copiarLista(respuesta.contenido, contenido, ':')) {
      this->bitacora.registrarAccion(this->nombre(), "lista truncada al tamaño máximo del mensaje");
    }
  }
  return respuesta;
}

Mensaje Servidor::imprimirProductos(Mensaje &respuesta, std::string &idCategoria, std::string &idBodega) {
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
    copiarTexto(respuesta.contenido, idCategoria.c_str());
  } else {
    respuesta.comando = Comando::Ok;
    // Si la lista no cabe completa se registra en la bitácora
    if (!copiarLista(respuesta.contenido, contenido, ':')) {
      this->bitacora.registrarAccion(this->nombre(), "lista truncada al tamaño máximo del mensaje");
    }
  }
  return respuesta;
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
    copiarTexto(respuesta.contenido, contenido.c_str());
  } else {
    respuesta.comando = Comando::ErrorNoDisponible;
    // Se usa el identificador del producto buscado, no un texto descriptivo
    copiarTexto(respuesta.contenido, solicitud.contenido);
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
  return imprimirCategorias(respuesta, categorias);
}

Mensaje Servidor::atenderProductos(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  // Separa la categoría y la bodega del contenido recibido
  std::vector<std::string> campos = separarCampos(solicitud.contenido, ':');
  std::string idCategoria = campos.size() > 0 ? campos[0] : "";
  std::string idBodega = campos.size() > 1 ? campos[1] : "";
  // Si se indicó bodega, verifica que exista
  if (!idBodega.empty()) {
    if (!this->bodegaExiste(idBodega)) {
      respuesta.comando = Comando::ErrorBodegaInexistente;
      copiarTexto(respuesta.contenido, idBodega.c_str());
      return respuesta;
    }
  }
  return imprimirProductos(respuesta, idCategoria, idBodega);
}

Mensaje Servidor::atenderExistencia(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  // Separa el producto y la bodega del contenido recibido
  std::vector<std::string> campos = separarCampos(solicitud.contenido, ':');
  std::string idProducto = campos.size() > 0 ? campos[0] : "";
  std::string idBodega = campos.size() > 1 ? campos[1] : "";
  if (!idBodega.empty()) {
    // Busca solo en la bodega indicada
    for (const ProductoTexto &producto : this->sistemaArchivos.listar_productos(idBodega)) {
      if (producto.producto == idProducto) {
        respuesta.comando = Comando::Si;
        copiarTexto(respuesta.contenido, producto.cantidad.c_str());
        return respuesta;
      }
    }
    respuesta.comando = Comando::No;
    copiarTexto(respuesta.contenido, "");
    return respuesta;
  }
  // Sin bodega indicada: busca en todas
  ProductoTexto producto;
  if (this->buscarEnTodasLasBodegas(idProducto, producto)) {
    respuesta.comando = Comando::Si;
    copiarTexto(respuesta.contenido, producto.cantidad.c_str());
  } else {
    respuesta.comando = Comando::No;
    copiarTexto(respuesta.contenido, "");
  }
  return respuesta;
}

Mensaje Servidor::atenderFiltrar(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  // Separa categoría, criterio y bodega del contenido recibido
  std::vector<std::string> campos = separarCampos(solicitud.contenido, ':');
  std::string idCategoria = campos.size() > 0 ? campos[0] : "";
  std::string criterio = campos.size() > 1 ? campos[1] : "";
  std::string idBodega = campos.size() > 2 ? campos[2] : "";
  std::vector<std::string> bodegasARevisar;
  // Corrobora si la bodega no está vacía para retirar el objeto solicitado
  if (!idBodega.empty()) {
    bodegasARevisar.push_back(idBodega);
  // En caso de no encontrarlo, la lista en las bodegas necesitadas de revisión
  } else {
    bodegasARevisar = this->sistemaArchivos.listar_bodegas();
  }
  return filtrar(respuesta, bodegasARevisar, idCategoria, criterio);
}

Mensaje Servidor::atenderCrearProducto(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  // Separa producto, categoría, bodega, cantidad y precio del contenido recibido
  std::vector<std::string> campos = separarCampos(solicitud.contenido, ':');
  if (campos.size() < 5) {
    respuesta.comando = Comando::ErrorFormatoInvalido;
    copiarTexto(respuesta.contenido, "CREAR_PRODUCTO");
    return respuesta;
  }
  std::string idProducto = campos[0], idCategoria = campos[1], idBodega = campos[2], cantidad = campos[3], precio = campos[4];
  // Verifica que la bodega exista antes de intentar insertar
  if (!this->bodegaExiste(idBodega)) {
    respuesta.comando = Comando::ErrorBodegaInexistente;
    copiarTexto(respuesta.contenido, idBodega.c_str());
    return respuesta;
  }
  // Verifica que el producto no exista ya en la bodega, porque el sistema de archivos no lo detecta
  if (this->productoEnBodega(idBodega, idProducto)) {
    respuesta.comando = Comando::ErrorProductoExistente;
    copiarTexto(respuesta.contenido, idProducto);
    return respuesta;
  }
  // Intenta insertar el producto
  if (this->sistemaArchivos.insertar_producto(idBodega, idCategoria, idProducto, cantidad, precio)) {
    respuesta.comando = Comando::Ok;
    std::string contenido = idProducto + ":" + idBodega;
    copiarTexto(respuesta.contenido, contenido);
  } else {
    // El sistema de archivos solo rechaza el producto si su registro excede el tamaño de un bloque
    respuesta.comando = Comando::ErrorFormatoInvalido;
    copiarTexto(respuesta.contenido, "CREAR_PRODUCTO");
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
    copiarTexto(respuesta.contenido, "ACTUALIZAR_PRODUCTO");
    return respuesta;
  }
  std::string idProducto = campos[0], idBodega = campos[1], cantidad = campos[2], precio = campos[3];
  // Verifica que la bodega exista antes de buscar el producto
  if (!this->bodegaExiste(idBodega)) {
    respuesta.comando = Comando::ErrorBodegaInexistente;
    copiarTexto(respuesta.contenido, idBodega);
    return respuesta;
  }
  // Se imprime la respuesta obtenida en el método actualizarProducto
  return actualizarProducto(respuesta, idProducto, idBodega, cantidad, precio);
}

Mensaje Servidor::atenderEliminarProducto(const Mensaje &solicitud) {
  // Variable local para crear el mensaje a enviar
  Mensaje respuesta;
  respuesta.idCliente = solicitud.idCliente;
  // Separa producto y bodega del contenido recibido
  std::vector<std::string> campos = separarCampos(solicitud.contenido, ':');
  if (campos.size() < 2) {
    respuesta.comando = Comando::ErrorFormatoInvalido;
    copiarTexto(respuesta.contenido, "ELIMINAR_PRODUCTO");
    return respuesta;
  }
  std::string idProducto = campos[0], idBodega = campos[1];
  // Verifica que la bodega exista antes de buscar el producto
  if (!this->bodegaExiste(idBodega)) {
    respuesta.comando = Comando::ErrorBodegaInexistente;
    copiarTexto(respuesta.contenido, idBodega);
    return respuesta;
  }
  if (this->sistemaArchivos.extraer_producto(idBodega, idProducto)) {
    respuesta.comando = Comando::Ok;
    std::string contenido = idProducto + ":" + idBodega;
    copiarTexto(respuesta.contenido, contenido.c_str());
  } else {
    respuesta.comando = Comando::ErrorNoDisponible;
    copiarTexto(respuesta.contenido, idProducto.c_str());
  }
  return respuesta;
}

void Servidor::ejecutar() {
  // Registro de bitácora: inicialización
  this->bitacora.registrarAccion(this->nombre(), "inicialización");
  // Crea el archivo de bodegas propio de este proceso, partiendo siempre limpio para pruebas
  this->sistemaArchivos.crear_archivo(this->rutaBodegas);
  // Prepara las bodegas y productos iniciales de este servidor
  this->preparar(this->sistemaArchivos);
  // Se ejecuta hasta recibir la solicitud de finalización
  while (true) {
    // Se recibe la solicitud del intermediario dirigida a este servidor
    Mensaje solicitud = this->colaDesdeIntermediario.desencolar(this->idServidor);
    // Registro de bitácora: desencolamiento de mensaje
    this->bitacora.registrarAccion(this->nombre(), "desencolamiento de mensaje");
    // Si se indica la operación "FIN", finaliza el servidor
    if (solicitud.idCliente == FIN) { break; }
    // Se crea la respuesta para el intermediario
    Mensaje respuesta = this->procesarSolicitud(solicitud);
    // Registro de bitácora: proceso de solicitud
    this->bitacora.registrarAccion(this->nombre(), "proceso de solicitud");
    // Se encola la respuesta para el intermediario, que siempre la espera con tipo 1
    this->colaHaciaIntermediario.encolar(respuesta, 1);
    // Registro de bitácora: encolamiento de mensaje
    this->bitacora.registrarAccion(this->nombre(), "encolamiento de mensaje");
  }
  // Cierra el archivo de bodegas de este proceso
  this->sistemaArchivos.cerrar_archivo();
  // Registro de bitácora: finalización
  this->bitacora.registrarAccion(this->nombre(), "finalización");
}
