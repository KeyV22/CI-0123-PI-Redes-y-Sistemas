/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Isla 3: Grupo 2
  *
  *  ******  Intermediario.cc: Implementación de la clase Intermediario (proceso independiente)
  *
  * (Fedora version)
  *
 **/

#include "Intermediario.h"

Intermediario::Intermediario(ColaMensajes &colaDesdeCliente, ColaMensajes &colaHaciaCliente, ColaMensajes &colaHaciaServidor, ColaMensajes &colaDesdeServidor, ColaMensajes &colaHaciaRemoto, ColaMensajes &colaDesdeRemoto, const std::vector<long> &idsServidores, Bitacora &bitacora) :
    colaDesdeCliente(colaDesdeCliente),
    colaHaciaCliente(colaHaciaCliente),
    colaHaciaServidor(colaHaciaServidor),
    colaDesdeServidor(colaDesdeServidor),
    idsServidores(idsServidores),
    bitacora(bitacora),
    enlace(colaHaciaRemoto, colaDesdeRemoto, bitacora),
    pid(-1) {}

void Intermediario::iniciar() {
  // Crea el proceso hijo que ejecutará el intermediario
  this->pid = fork();
  // Si no se pudo crear el proceso se informa el error
  if (this->pid == -1) {
    std::cerr << "No se pudo crear el proceso del Intermediario" << std::endl;
  // Código que solo ejecuta el proceso hijo
  } else if (this->pid == 0) {
    // Ejecuta el ciclo principal del intermediario
    try {
      // Ejecuta el ciclo del proceso
      this->ejecutar();
    // Si falla una cola se informa y se termina el proceso con error
    } catch (const std::exception &excepcion) {
      std::cerr << "Intermediario" << ": " << excepcion.what() << std::endl;
      _exit(1);
    }
    // Termina el proceso hijo sin ejecutar destructores que afecten recursos del padre
    _exit(0);
  }
}

void Intermediario::detener() {
  // Si el proceso fue creado, se le pide finalizar y se espera su cierre
  if (this->pid > 0) {
    // Se crea un mensaje para finalizar el intermediario
    Mensaje fin;
    // Se limpia el mensaje para no enviar basura
    std::memset(&fin, 0, sizeof(Mensaje));
    // Se declara la operación "FIN"
    fin.idCliente = FIN;
    // Se indica el comando
    fin.comando = Comando::Fin;
    // Se copia el contenido indicando el final
    copiarTexto(fin.contenido, "TERMINAR");
    // Se encola el mensaje en la cola de solicitudes de los clientes
    this->colaDesdeCliente.encolar(fin, 1);
    // Registro de bitácora: encolamiento de operación final
    this->bitacora.registrarAccion("Intermediario", "encolamiento de operación final");
    // Espera la finalización del proceso hijo
    waitpid(this->pid, nullptr, 0);
    // Indica que ya no hay un proceso activo
    this->pid = -1;
  }
}

Mensaje Intermediario::consultarServidor(long idServidor, const Mensaje &solicitud) {
  // Se encola la solicitud dirigida al servidor indicado
  this->colaHaciaServidor.encolar(solicitud, idServidor);
  // Registro de bitácora: encolamiento de mensaje
  this->bitacora.registrarAccion("Intermediario", "encolamiento de mensaje");
  // Se espera la respuesta del servidor, que siempre llega con tipo 1
  Mensaje respuesta = this->colaDesdeServidor.desencolar(1);
  // Registro de bitácora: desencolamiento de mensaje
  this->bitacora.registrarAccion("Intermediario", "desencolamiento de mensaje");
  // Se retorna la respuesta del servidor
  return respuesta;
}

Mensaje Intermediario::buscarEnServidores(const Mensaje &solicitud, Comando exito) {
  // Respuesta que se retorna si ningún servidor tiene lo buscado
  Mensaje ultima;
  // Se limpia el mensaje para no retornar basura
  std::memset(&ultima, 0, sizeof(Mensaje));
  // Se asigna el cliente al que se le responde
  ultima.idCliente = solicitud.idCliente;
  // Se indica el comando de error
  ultima.comando = Comando::ErrorFormatoInvalido;
  // Se indica el motivo del error
  copiarTexto(ultima.contenido, "SERVIDORES");
  // Se pregunta a cada servidor en orden
  for (long idServidor : this->idsServidores) {
    // Se consulta al servidor actual
    ultima = this->consultarServidor(idServidor, solicitud);
    // Si el servidor tiene lo buscado, no hace falta preguntar a los demás
    if (ultima.comando == exito) { return ultima; }
  }
  // Si ningún servidor lo tiene se retorna la última respuesta
  return ultima;
}

std::vector<Mensaje> Intermediario::consultarTodos(const Mensaje &solicitud) {
  // Respuestas de todos los servidores
  std::vector<Mensaje> respuestas;
  // Se pregunta a todos los servidores
  for (long idServidor : this->idsServidores) {
    // Se guarda la respuesta del servidor actual
    respuestas.push_back(this->consultarServidor(idServidor, solicitud));
  }
  // Se retornan las respuestas en el orden de los servidores
  return respuestas;
}

std::vector<std::string> Intermediario::reunirElementos(const std::vector<Mensaje> &respuestas, Comando exito) {
  // Elementos de las listas de éxito, sin repetir
  std::vector<std::string> elementos;
  // Se recorren las respuestas para reunir los elementos de las listas
  for (const Mensaje &respuesta : respuestas) {
    // Solo se unen las respuestas de éxito; "NINGUNA" indica una lista vacía
    if (respuesta.comando == exito && std::string(respuesta.contenido) != "NINGUNA") {
      // Se recorren los elementos de la lista del servidor
      for (const std::string &elemento : separarCampos(respuesta.contenido, ':')) {
        // Se agrega el elemento solo si todavía no estaba
        if (!contiene(elementos, elemento)) { elementos.push_back(elemento); }
      }
    }
  }
  // Se retornan los elementos reunidos
  return elementos;
}

Mensaje Intermediario::armarLista(const Mensaje &solicitud, Comando exito, const std::vector<std::string> &elementos) {
  // Respuesta que se retorna al cliente
  Mensaje lista;
  // Se limpia el mensaje para no retornar basura
  std::memset(&lista, 0, sizeof(Mensaje));
  // Se asigna el cliente al que se le responde
  lista.idCliente = solicitud.idCliente;
  // Se indica el comando de éxito
  lista.comando = exito;
  // Texto con los elementos separados por ':'
  std::string contenido;
  // Se une cada elemento
  for (size_t i = 0; i < elementos.size(); i++) {
    if (i > 0) { contenido += ":"; }
    contenido += elementos[i];
  }
  // Se copia la lista unida y si no cabe se corta entre elementos para no dejar uno partido
  copiarLista(lista.contenido, contenido, ':');
  // Se retorna la lista armada
  return lista;
}

Mensaje Intermediario::elegirRespuesta(const Mensaje &solicitud, const std::vector<Mensaje> &respuestas) {
  // Gana la primera respuesta que no sea de bodega inexistente
  for (const Mensaje &respuesta : respuestas) {
    if (respuesta.comando != Comando::ErrorBodegaInexistente) { return respuesta; }
  }
  // Todos los servidores respondieron bodega inexistente
  if (!respuestas.empty()) { return respuestas[0]; }
  // Respuesta de error para el caso sin servidores
  Mensaje error;
  // Se limpia el mensaje para no retornar basura
  std::memset(&error, 0, sizeof(Mensaje));
  // Se asigna el cliente al que se le responde
  error.idCliente = solicitud.idCliente;
  // Se indica el comando de error
  error.comando = Comando::ErrorFormatoInvalido;
  // Se indica el motivo del error
  copiarTexto(error.contenido, "SERVIDORES");
  // Se retorna el error
  return error;
}

Mensaje Intermediario::unirRespuestas(const Mensaje &solicitud, Comando exito) {
  // Se pregunta a todos los servidores
  std::vector<Mensaje> respuestas = this->consultarTodos(solicitud);
  // Se reúnen los elementos de las listas de éxito
  std::vector<std::string> elementos = this->reunirElementos(respuestas, exito);
  // Si hubo elementos se retorna la lista unida
  if (!elementos.empty()) { return this->armarLista(solicitud, exito, elementos); }
  // Sin elementos se elige la respuesta más informativa
  return this->elegirRespuesta(solicitud, respuestas);
}

Mensaje Intermediario::aplicarCambio(const Mensaje &solicitud) {
  // Respuesta que se retorna si ningún servidor tiene la bodega
  Mensaje ultima;
  // Se limpia el mensaje para no retornar basura
  std::memset(&ultima, 0, sizeof(Mensaje));
  // Se asigna el cliente al que se le responde
  ultima.idCliente = solicitud.idCliente;
  // Se indica el comando de error
  ultima.comando = Comando::ErrorFormatoInvalido;
  // Se indica el motivo del error
  copiarTexto(ultima.contenido, "SERVIDORES");
  // Se pregunta a cada servidor en orden
  for (long idServidor : this->idsServidores) {
    // Se consulta al servidor actual
    ultima = this->consultarServidor(idServidor, solicitud);
    // El primer servidor que no responda "bodega inexistente" es el dueño de la bodega y su respuesta es la definitiva
    if (ultima.comando != Comando::ErrorBodegaInexistente) { return ultima; }
  }
  // Si ningún servidor tiene la bodega se retorna el error del último servidor
  return ultima;
}

Mensaje Intermediario::atenderConectar(const Mensaje &solicitud) {
  // Se registra la sesión del cliente; si ya existía, el conjunto no cambia
  this->sesiones.insert(solicitud.idCliente);
  // Registro de bitácora: sesión abierta
  this->bitacora.registrarAccion("Intermediario", "sesión abierta del cliente " + std::to_string(solicitud.idCliente));
  // Se responde OK sin contenido
  return this->armarLista(solicitud, Comando::Ok, {});
}

Mensaje Intermediario::atenderDesconectar(const Mensaje &solicitud) {
  // Se intenta cerrar la sesión; erase retorna la cantidad de sesiones eliminadas
  if (this->sesiones.erase(solicitud.idCliente) == 0) {
    // Registro de bitácora: sesión inválida
    this->bitacora.registrarAccion("Intermediario", "desconexión sin sesión del cliente " + std::to_string(solicitud.idCliente));
    // El cliente no tenía una sesión abierta
    return this->armarLista(solicitud, Comando::ErrorSesionInvalida, {});
  }
  // Registro de bitácora: sesión cerrada
  this->bitacora.registrarAccion("Intermediario", "sesión cerrada del cliente " + std::to_string(solicitud.idCliente));
  // Se responde OK sin contenido
  return this->armarLista(solicitud, Comando::Ok, {});
}

bool Intermediario::tieneBodega(const Mensaje &solicitud) {
  // Si el contenido tiene un separador ':' la solicitud indica una bodega concreta
  return std::string(solicitud.contenido).find(':') != std::string::npos;
}

Mensaje Intermediario::buscarConRespaldoRemoto(const Mensaje &solicitud, Comando exito) {
  // Primero se busca en los servidores locales
  Mensaje local = this->buscarEnServidores(solicitud, exito);
  // Si algún servidor local lo tiene no hace falta consultar la isla remota
  if (local.comando == exito) { return local; }
  // Una solicitud con bodega indicada solo se atiende con los servidores locales
  if (this->tieneBodega(solicitud)) { return local; }
  // Categoría y producto que la isla remota informe
  std::string categoria;
  EnlaceMancomunado::ProductoIsla producto;
  // Se busca el producto en la isla remota usando el nombre en minúsculas, como lo exige el protocolo
  if (!this->enlace.buscarProducto(aMinusculas(solicitud.contenido), categoria, producto)) { return local; }
  // Para BuscarProducto se informa bodega, categoría, producto, cantidad y precio, igual que un servidor local
  if (exito == Comando::Disponible)
    return this->armarLista(solicitud, Comando::Disponible, {"isla2", categoria, producto.nombre, std::to_string(producto.existencia), producto.precio});
  // Para Existencia se informa solo la cantidad
  else
    return this->armarLista(solicitud, Comando::Si, {std::to_string(producto.existencia)});
}

Mensaje Intermediario::unirCategorias(const Mensaje &solicitud) {
  // Se pregunta a todos los servidores locales
  std::vector<Mensaje> respuestas = this->consultarTodos(solicitud);
  // Se reúnen las categorías locales
  std::vector<std::string> elementos = this->reunirElementos(respuestas, Comando::Ok);
  // Categorías de la isla remota
  std::vector<std::string> remotas;
  // Si la isla remota respondió, se agregan sus categorías sin repetir
  if (this->enlace.obtenerCategorias(remotas)) {
    for (const std::string &categoria : remotas) {
      if (!contiene(elementos, categoria)) { elementos.push_back(categoria); }
    }
  }
  // Si hubo categorías se retorna la lista unida
  if (!elementos.empty()) { return this->armarLista(solicitud, Comando::Ok, elementos); }
  // Sin categorías se elige la respuesta más informativa
  return this->elegirRespuesta(solicitud, respuestas);
}

Mensaje Intermediario::listarProductos(const Mensaje &solicitud) {
  // Primero se consulta a los servidores locales
  Mensaje local = this->unirRespuestas(solicitud, Comando::Ok);
  // Si hubo productos locales se retornan
  if (local.comando == Comando::Ok) { return local; }
  // Una solicitud con bodega indicada solo se atiende con los servidores locales
  if (this->tieneBodega(solicitud)) { return local; }
  // Productos de la categoría en la isla remota
  std::vector<EnlaceMancomunado::ProductoIsla> productos;
  // Si la isla remota no tiene la categoría se retorna la respuesta local
  if (!this->enlace.obtenerProductos(aMinusculas(solicitud.contenido), productos) || productos.empty()) { return local; }
  // Nombres de los productos de la isla remota
  std::vector<std::string> nombres;
  for (const EnlaceMancomunado::ProductoIsla &producto : productos) {
    nombres.push_back(producto.nombre);
  }
  // Se retorna la lista de productos de la isla remota
  return this->armarLista(solicitud, Comando::Ok, nombres);
}

Mensaje Intermediario::resolverSolicitud(const Mensaje &solicitud) {
  // Se evalúa el comando de la solicitud y se delega al método correspondiente
  if (solicitud.comando == Comando::Conectar)
    return this->atenderConectar(solicitud);
  else if (solicitud.comando == Comando::Desconectar)
    return this->atenderDesconectar(solicitud);
  else if (solicitud.comando == Comando::BuscarProducto)
    return this->buscarConRespaldoRemoto(solicitud, Comando::Disponible);
  else if (solicitud.comando == Comando::Existencia)
    return this->buscarConRespaldoRemoto(solicitud, Comando::Si);
  else if (solicitud.comando == Comando::Categorias)
    return this->unirCategorias(solicitud);
  else if (solicitud.comando == Comando::Productos)
    return this->listarProductos(solicitud);
  else if (solicitud.comando == Comando::Filtrar)
    return this->unirRespuestas(solicitud, Comando::Coincidencia);
  else if (solicitud.comando == Comando::CrearProducto || solicitud.comando == Comando::ActualizarProducto || solicitud.comando == Comando::EliminarProducto)
    return this->aplicarCambio(solicitud);
  else
    // En caso de comando no soportado atiende el primer servidor que responde con el error de formato
    return this->consultarServidor(this->idsServidores[0], solicitud);
}

Mensaje Intermediario::procesarSolicitud(const Mensaje &mensaje) {
  // Copia base de la respuesta
  Mensaje respuesta = mensaje;
  // Variable local donde se arma el texto final que verá el cliente
  std::string textoFinal;
  // Se traduce el comando de resultado del Servidor al formato textual del protocolo
  if (mensaje.comando == Comando::Ok && std::string(mensaje.contenido).empty())
    textoFinal = "OK";
  else if (mensaje.comando == Comando::Ok)
    textoFinal = "OK " + reemplazarSeparador(mensaje.contenido, ':', ',');
  else if (mensaje.comando == Comando::OkCatalogo)
    textoFinal = "OK_CATALOGO";
  else if (mensaje.comando == Comando::Disponible)
    textoFinal = "DISPONIBLE " + reemplazarSeparador(mensaje.contenido, ':', ',');
  else if (mensaje.comando == Comando::Si)
    textoFinal = "SI " + std::string(mensaje.contenido);
  else if (mensaje.comando == Comando::No)
    textoFinal = "NO";
  else if (mensaje.comando == Comando::Coincidencia)
    textoFinal = "COINCIDENCIA " + reemplazarSeparador(mensaje.contenido, ':', ',');
  else if (mensaje.comando == Comando::SinCoincidencia)
    textoFinal = "SIN_COINCIDENCIA";
  else if (mensaje.comando == Comando::Bloqueado)
    textoFinal = "BLOQUEADO " + std::string(mensaje.contenido);
  else if (mensaje.comando == Comando::ErrorFormatoInvalido)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " FORMATO_INVALIDO";
  else if (mensaje.comando == Comando::ErrorSesionInvalida)
    textoFinal = "ERROR SESION_INVALIDA";
  else if (mensaje.comando == Comando::ErrorCategoriaInexistente)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " CATEGORIA_INEXISTENTE";
  else if (mensaje.comando == Comando::ErrorCategoriaExistente)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " CATEGORIA_EXISTENTE";
  else if (mensaje.comando == Comando::ErrorCategoriaNoVacia)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " CATEGORIA_NO_VACIA";
  else if (mensaje.comando == Comando::ErrorBodegaInexistente)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " BODEGA_INEXISTENTE";
  else if (mensaje.comando == Comando::ErrorProductoExistente)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " PRODUCTO_EXISTENTE";
  else if (mensaje.comando == Comando::ErrorNoDisponible)
    textoFinal = "ERROR " + std::string(mensaje.contenido) + " NO_DISPONIBLE";
  else if (mensaje.comando == Comando::ErrorPermisoDenegado)
    textoFinal = "ERROR PERMISO_DENEGADO";
  else if (mensaje.comando == Comando::ErrorRecursoBloqueado)
    textoFinal = "ERROR RECURSO_BLOQUEADO";
  else
    textoFinal = "ERROR FORMATO_INVALIDO";
  // Se copia el texto final al contenido del mensaje para el cliente; si es una lista que no cabe se corta entre elementos
  copiarLista(respuesta.contenido, textoFinal, ',');
  // Se retorna la respuesta traducida
  return respuesta;
}

void Intermediario::finalizarServidores() {
  // Se crea un mensaje para finalizar los servidores
  Mensaje fin;
  // Se limpia el mensaje para no enviar basura
  std::memset(&fin, 0, sizeof(Mensaje));
  // Se declara la operación "FIN"
  fin.idCliente = FIN;
  // Se indica el comando
  fin.comando = Comando::Fin;
  // Se copia el contenido indicando el final
  copiarTexto(fin.contenido, "TERMINAR");
  // Se envía el mensaje a cada servidor
  for (long idServidor : this->idsServidores) {
    // Se encola el mensaje de finalización dirigido al servidor actual
    this->colaHaciaServidor.encolar(fin, idServidor);
    // Registro de bitácora: encolamiento de operación final
    this->bitacora.registrarAccion("Intermediario", "encolamiento de operación final");
  }
}

void Intermediario::ejecutar() {
  // Registro de bitácora: inicialización
  this->bitacora.registrarAccion("Intermediario", "inicialización");
  // Se ejecuta hasta recibir la solicitud de finalización
  while (true) {
    // Se recibe la solicitud de cualquier cliente
    Mensaje solicitud = this->colaDesdeCliente.desencolar(1);
    // Registro de bitácora: desencolamiento de mensaje
    this->bitacora.registrarAccion("Intermediario", "desencolamiento de mensaje");
    // Si se indica la operación "FIN", finaliza el intermediario
    if (solicitud.idCliente == FIN) { break; }
    // Una solicitud sin un cliente válido no se puede responder, por lo que se descarta
    if (solicitud.idCliente < 1) {
      // Registro de bitácora: descarte de solicitud
      this->bitacora.registrarAccion("Intermediario", "solicitud descartada por cliente inválido");
      continue;
    }
    // Se consulta a los servidores según el comando
    Mensaje respuesta = this->resolverSolicitud(solicitud);
    // Se crea la respuesta para el cliente
    Mensaje mensaje = this->procesarSolicitud(respuesta);
    // Registro de bitácora: proceso de solicitud
    this->bitacora.registrarAccion("Intermediario", "proceso de solicitud");
    // Se encola la respuesta para el cliente correspondiente, identificado por el tipo del mensaje
    this->colaHaciaCliente.encolar(mensaje, mensaje.idCliente);
    // Registro de bitácora: encolamiento de mensaje
    this->bitacora.registrarAccion("Intermediario", "encolamiento de mensaje");
  }
  // Se pide a los servidores que finalicen
  this->finalizarServidores();
  // Se pide a la isla remota que finalice
  this->enlace.finalizar();
  // Registro de bitácora: finalización
  this->bitacora.registrarAccion("Intermediario", "finalización");
}
