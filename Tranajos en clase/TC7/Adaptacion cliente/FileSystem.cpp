#include "FileSystem.hpp"
#include <cstring>
#include <ctime>
#include <iostream>
#include <stdexcept>


// Serializacion / deserializacion: cada struct sabe convertirse a bytes y
// reconstruirse desde bytes, respetando los offsets exactos del documento.
// Se hace con memcpy campo por campo (en vez de "struct empacado" del
// compilador) para que el formato en disco no dependa de como cada compilador
// decida alinear la memoria.
void SuperBloque::serializar(char* buffer) const {
    memset(buffer, 0, TAMANO_BLOQUE);
    int off = 0;
    memcpy(buffer + off, firma, 4); off += 4;
    memcpy(buffer + off, &tamanoBloque, 2); off += 2;
    memcpy(buffer + off, &cantidadBloques, 4); off += 4;
    memcpy(buffer + off, &punteroBitmap, 4); off += 4;
    memcpy(buffer + off, &punteroIndiceCategorias, 4); off += 4;
    memcpy(buffer + off, &punteroIndiceOrdenes, 4); off += 4;
}

void SuperBloque::deserializar(const char* buffer) {
    int off = 0;
    memcpy(firma, buffer + off, 4); off += 4;
    memcpy(&tamanoBloque, buffer + off, 2); off += 2;
    memcpy(&cantidadBloques, buffer + off, 4); off += 4;
    memcpy(&punteroBitmap, buffer + off, 4); off += 4;
    memcpy(&punteroIndiceCategorias, buffer + off, 4); off += 4;
    memcpy(&punteroIndiceOrdenes, buffer + off, 4); off += 4;
}

void BloqueIndiceCategorias::serializar(char* buffer) const {
    memset(buffer, 0, TAMANO_BLOQUE);
    int off = 0;
    for (auto& e : entradas) {
        memcpy(buffer + off, &e.idCategoria, 1); off += 1;
        memcpy(buffer + off, e.nombreCategoria, 16); off += 16;
        memcpy(buffer + off, &e.bloqueInicio, 4); off += 4;
    }
    memcpy(buffer + (TAMANO_BLOQUE - 4), &siguienteBloque, 4);
}

void BloqueIndiceCategorias::deserializar(const char* buffer) {
    entradas.clear();
    int off = 0;
    for (int i = 0; i < MAX_ENTRADAS_INDICE_CAT; i++) {
        EntradaIndiceCategoria e{};
        memcpy(&e.idCategoria, buffer + off, 1); off += 1;
        memcpy(e.nombreCategoria, buffer + off, 16); off += 16;
        memcpy(&e.bloqueInicio, buffer + off, 4); off += 4;
        if (e.nombreCategoria[0] == '\0' && e.bloqueInicio == 0) continue; // entrada vacia
        entradas.push_back(e);
    }
    memcpy(&siguienteBloque, buffer + (TAMANO_BLOQUE - 4), 4);
}

void BloqueCategoria::serializar(char* buffer) const {
    memset(buffer, 0, TAMANO_BLOQUE);
    int off = 0;
    uint8_t cantidad = static_cast<uint8_t>(productos.size());
    memcpy(buffer + off, &idCategoria, 1); off += 1;
    memcpy(buffer + off, &siguienteBloque, 4); off += 4;
    memcpy(buffer + off, &cantidad, 1); off += 1;
    for (auto& p : productos) {
        memcpy(buffer + off, &p.idProducto, 4); off += 4;
        memcpy(buffer + off, &p.precio, 4); off += 4;
        memcpy(buffer + off, &p.cantidadDisponible, 4); off += 4;
        memcpy(buffer + off, p.nombre, 20); off += 20;
    }
}

void BloqueCategoria::deserializar(const char* buffer) {
    productos.clear();
    int off = 0;
    uint8_t cantidad;
    memcpy(&idCategoria, buffer + off, 1); off += 1;
    memcpy(&siguienteBloque, buffer + off, 4); off += 4;
    memcpy(&cantidad, buffer + off, 1); off += 1;
    for (int i = 0; i < cantidad; i++) {
        Producto p{};
        memcpy(&p.idProducto, buffer + off, 4); off += 4;
        memcpy(&p.precio, buffer + off, 4); off += 4;
        memcpy(&p.cantidadDisponible, buffer + off, 4); off += 4;
        memcpy(p.nombre, buffer + off, 20); off += 20;
        productos.push_back(p);
    }
}

void BloqueIndiceOrdenes::serializar(char* buffer) const {
    memset(buffer, 0, TAMANO_BLOQUE);
    int off = 0;
    for (auto& e : entradas) {
        memcpy(buffer + off, &e.idOrden, 4); off += 4;
        memcpy(buffer + off, &e.bloqueInicio, 4); off += 4;
        memcpy(buffer + off, &e.timestamp, 4); off += 4;
    }
    memcpy(buffer + (TAMANO_BLOQUE - 4), &siguienteBloque, 4);
}

void BloqueIndiceOrdenes::deserializar(const char* buffer) {
    entradas.clear();
    int off = 0;
    for (int i = 0; i < MAX_ENTRADAS_INDICE_ORD; i++) {
        EntradaIndiceOrden e{};
        memcpy(&e.idOrden, buffer + off, 4); off += 4;
        memcpy(&e.bloqueInicio, buffer + off, 4); off += 4;
        memcpy(&e.timestamp, buffer + off, 4); off += 4;
        if (e.idOrden == 0 && e.bloqueInicio == 0) continue;
        entradas.push_back(e);
    }
    memcpy(&siguienteBloque, buffer + (TAMANO_BLOQUE - 4), 4);
}

void BloqueOrden::serializar(char* buffer) const {
    memset(buffer, 0, TAMANO_BLOQUE);
    int off = 0;
    uint16_t cantidad = static_cast<uint16_t>(items.size());
    memcpy(buffer + off, &idOrden, 4); off += 4;
    memcpy(buffer + off, &siguienteBloque, 4); off += 4;
    memcpy(buffer + off, &cantidad, 2); off += 2;
    for (auto& it : items) {
        memcpy(buffer + off, &it.idProducto, 4); off += 4;
        memcpy(buffer + off, &it.cantidad, 2); off += 2;
        memcpy(buffer + off, &it.precioUnitario, 4); off += 4;
    }
}

void BloqueOrden::deserializar(const char* buffer) {
    items.clear();
    int off = 0;
    uint16_t cantidad;
    memcpy(&idOrden, buffer + off, 4); off += 4;
    memcpy(&siguienteBloque, buffer + off, 4); off += 4;
    memcpy(&cantidad, buffer + off, 2); off += 2;
    for (int i = 0; i < cantidad; i++) {
        ItemOrden it{};
        memcpy(&it.idProducto, buffer + off, 4); off += 4;
        memcpy(&it.cantidad, buffer + off, 2); off += 2;
        memcpy(&it.precioUnitario, buffer + off, 4); off += 4;
        items.push_back(it);
    }
}

// FileSystem

FileSystem::FileSystem(const std::string& archivo) {
    disco.open(archivo, std::ios::in | std::ios::out | std::ios::binary);
    if (!disco.is_open()) {
        // El archivo no existe todavia: crearlo vacio y luego inicializarlo
        disco.open(archivo, std::ios::out | std::ios::binary);
        disco.close();
        disco.open(archivo, std::ios::in | std::ios::out | std::ios::binary);
        inicializarArchivo();
    } else {
        // Ya existia: leer el superbloque para conocer los punteros actuales
        char buffer[TAMANO_BLOQUE];
        leerBloque(BLOQUE_SUPERBLOQUE, buffer);
        superbloque.deserializar(buffer);
        if (strncmp(superbloque.firma, FIRMA, 4) != 0) {
            throw std::runtime_error("Archivo invalido: la firma no coincide con el formato TicAmazon");
        }
    }
}

FileSystem::~FileSystem() {
    if (disco.is_open()) disco.close();
}

void FileSystem::inicializarArchivo() {
    char buffer[TAMANO_BLOQUE];

    // --- Superbloque ---
    memcpy(superbloque.firma, FIRMA, 4);
    superbloque.tamanoBloque = TAMANO_BLOQUE;
    superbloque.cantidadBloques = 2048; // limite que soporta un bitmap de 256 bytes (2048 bits)
    superbloque.punteroBitmap = BLOQUE_BITMAP;
    superbloque.punteroIndiceCategorias = BLOQUE_INICIAL_CATEGORIAS;
    superbloque.punteroIndiceOrdenes = BLOQUE_INICIAL_ORDENES;
    superbloque.serializar(buffer);
    escribirBloque(BLOQUE_SUPERBLOQUE, buffer);

    // --- Bitmap: marcar ocupados los 4 bloques fijos que ya usamos ---
    memset(buffer, 0, TAMANO_BLOQUE);
    escribirBloque(BLOQUE_BITMAP, buffer); // primero todo en 0 (libre)
    // Ahora, usando la propia logica de bits, marcamos 0,1,2,3 como ocupados
    leerBloque(BLOQUE_BITMAP, buffer);
    for (uint32_t b = 0; b <= BLOQUE_INICIAL_ORDENES; b++) {
        buffer[b / 8] |= (1 << (b % 8));
    }
    escribirBloque(BLOQUE_BITMAP, buffer);

    // --- Indice de categorias vacio (directorio principal) ---
    BloqueIndiceCategorias indiceCat;
    indiceCat.serializar(buffer);
    escribirBloque(BLOQUE_INICIAL_CATEGORIAS, buffer);

    // --- Indice de ordenes vacio ---
    BloqueIndiceOrdenes indiceOrd;
    indiceOrd.serializar(buffer);
    escribirBloque(BLOQUE_INICIAL_ORDENES, buffer);
}

void FileSystem::leerBloque(uint32_t numeroBloque, char* buffer) {
    disco.seekg(numeroBloque * TAMANO_BLOQUE);
    disco.read(buffer, TAMANO_BLOQUE);
}

void FileSystem::escribirBloque(uint32_t numeroBloque, const char* buffer) {
    disco.seekp(numeroBloque * TAMANO_BLOQUE);
    disco.write(buffer, TAMANO_BLOQUE);
    disco.flush();
}

uint32_t FileSystem::asignarBloque() {
    char buffer[TAMANO_BLOQUE];
    leerBloque(BLOQUE_BITMAP, buffer);
    for (int byteIdx = 0; byteIdx < TAMANO_BLOQUE; byteIdx++) {
        if ((unsigned char)buffer[byteIdx] == 0xFF) continue; // los 8 bits de este byte ya estan ocupados
        for (int bit = 0; bit < 8; bit++) {
            if (!(buffer[byteIdx] & (1 << bit))) {
                buffer[byteIdx] |= (1 << bit);
                escribirBloque(BLOQUE_BITMAP, buffer);
                return byteIdx * 8 + bit;
            }
        }
    }
    throw std::runtime_error("DISCO_LLENO: no hay bloques libres en el bitmap");
}

void FileSystem::liberarBloque(uint32_t numeroBloque) {
    char buffer[TAMANO_BLOQUE];
    leerBloque(BLOQUE_BITMAP, buffer);
    buffer[numeroBloque / 8] &= ~(1 << (numeroBloque % 8));
    escribirBloque(BLOQUE_BITMAP, buffer);
}

// Directorio principal (indice de categorias)

uint32_t FileSystem::buscarBloqueInicioCategoria(uint8_t idCategoria) {
    uint32_t actual = superbloque.punteroIndiceCategorias;
    char buffer[TAMANO_BLOQUE];
    while (actual != FIN_CADENA) {
        leerBloque(actual, buffer);
        BloqueIndiceCategorias indice;
        indice.deserializar(buffer);
        for (auto& e : indice.entradas) {
            if (e.idCategoria == idCategoria) return e.bloqueInicio;
        }
        actual = indice.siguienteBloque;
    }
    return FIN_CADENA;
}

void FileSystem::registrarCategoriaEnIndice(uint8_t idCategoria, const std::string& nombre, uint32_t bloqueInicio) {
    uint32_t actual = superbloque.punteroIndiceCategorias;
    char buffer[TAMANO_BLOQUE];

    while (true) {
        leerBloque(actual, buffer);
        BloqueIndiceCategorias indice;
        indice.deserializar(buffer);

        if ((int)indice.entradas.size() < MAX_ENTRADAS_INDICE_CAT) {
            EntradaIndiceCategoria nueva{};
            nueva.idCategoria = idCategoria;
            memset(nueva.nombreCategoria, 0, 16);
            strncpy(nueva.nombreCategoria, nombre.c_str(), 15);
            nueva.bloqueInicio = bloqueInicio;
            indice.entradas.push_back(nueva);
            indice.serializar(buffer);
            escribirBloque(actual, buffer);
            return;
        }

        // Este bloque de indice esta lleno: el directorio principal crece
        // encadenando un bloque de indice adicional.
        if (indice.siguienteBloque == FIN_CADENA) {
            uint32_t nuevoBloque = asignarBloque();
            indice.siguienteBloque = nuevoBloque;
            indice.serializar(buffer);
            escribirBloque(actual, buffer);

            BloqueIndiceCategorias vacio;
            vacio.serializar(buffer);
            escribirBloque(nuevoBloque, buffer);
        }
        actual = indice.siguienteBloque;
    }
}


// Bodega: agregar / listar / reservar productos

void FileSystem::agregarProducto(uint8_t idCategoria, const std::string& nombreCategoria, const Producto& producto) {
    uint32_t bloqueInicio = buscarBloqueInicioCategoria(idCategoria);
    char buffer[TAMANO_BLOQUE];

    if (bloqueInicio == FIN_CADENA) {
        // Categoria nueva: crear su primer bloque y registrarla en el directorio
        bloqueInicio = asignarBloque();
        BloqueCategoria nuevoBloque;
        nuevoBloque.idCategoria = idCategoria;
        nuevoBloque.productos.push_back(producto);
        nuevoBloque.serializar(buffer);
        escribirBloque(bloqueInicio, buffer);
        registrarCategoriaEnIndice(idCategoria, nombreCategoria, bloqueInicio);
        return;
    }

    // Categoria existente: recorrer la cadena hasta encontrar espacio libre
    uint32_t actual = bloqueInicio;
    while (true) {
        leerBloque(actual, buffer);
        BloqueCategoria bloque;
        bloque.deserializar(buffer);

        if ((int)bloque.productos.size() < MAX_PRODUCTOS_POR_BLOQUE) {
            bloque.productos.push_back(producto);
            bloque.serializar(buffer);
            escribirBloque(actual, buffer);
            return;
        }

        if (bloque.siguienteBloque == FIN_CADENA) {
            // El bloque esta lleno: la bodega de esta categoria crece
            // encadenando un bloque adicional.
            uint32_t nuevoBloque = asignarBloque();
            bloque.siguienteBloque = nuevoBloque;
            bloque.serializar(buffer);
            escribirBloque(actual, buffer);

            BloqueCategoria siguiente;
            siguiente.idCategoria = idCategoria;
            siguiente.productos.push_back(producto);
            siguiente.serializar(buffer);
            escribirBloque(nuevoBloque, buffer);
            return;
        }
        actual = bloque.siguienteBloque;
    }
}

std::vector<Producto> FileSystem::listarProductos(uint8_t idCategoria) {
    std::vector<Producto> resultado;
    uint32_t actual = buscarBloqueInicioCategoria(idCategoria);
    char buffer[TAMANO_BLOQUE];

    while (actual != FIN_CADENA) {
        leerBloque(actual, buffer);
        BloqueCategoria bloque;
        bloque.deserializar(buffer);
        for (auto& p : bloque.productos) resultado.push_back(p);
        actual = bloque.siguienteBloque;
    }
    return resultado;
}

std::vector<std::string> FileSystem::listarCategorias() {
    std::vector<std::string> resultado;
    uint32_t actual = superbloque.punteroIndiceCategorias;
    char buffer[TAMANO_BLOQUE];

    while (actual != FIN_CADENA) {
        leerBloque(actual, buffer);
        BloqueIndiceCategorias indice;
        indice.deserializar(buffer);
        for (auto& e : indice.entradas) resultado.push_back(e.nombreCategoria);
        actual = indice.siguienteBloque;
    }
    return resultado;
}

bool FileSystem::reservarProducto(uint32_t idProducto, uint32_t cantidad, std::string* motivoError) {
    // Recorre el directorio principal, y dentro de cada categoria su cadena
    // de bloques, buscando el producto por id.
    uint32_t actualIndice = superbloque.punteroIndiceCategorias;
    char bufferIndice[TAMANO_BLOQUE];
    char bufferBloque[TAMANO_BLOQUE];

    while (actualIndice != FIN_CADENA) {
        leerBloque(actualIndice, bufferIndice);
        BloqueIndiceCategorias indice;
        indice.deserializar(bufferIndice);

        for (auto& entrada : indice.entradas) {
            uint32_t actualBloque = entrada.bloqueInicio;
            while (actualBloque != FIN_CADENA) {
                leerBloque(actualBloque, bufferBloque);
                BloqueCategoria bloque;
                bloque.deserializar(bufferBloque);

                for (auto& p : bloque.productos) {
                    if (p.idProducto == idProducto) {
                        if (p.cantidadDisponible < cantidad) {
                            if (motivoError) *motivoError = "STOCK_INSUFICIENTE";
                            return false;
                        }
                        p.cantidadDisponible -= cantidad;
                        bloque.serializar(bufferBloque);
                        escribirBloque(actualBloque, bufferBloque);
                        return true;
                    }
                }
                actualBloque = bloque.siguienteBloque;
            }
        }
        actualIndice = indice.siguienteBloque;
    }

    if (motivoError) *motivoError = "404";
    return false;
}

// Ordenes
uint32_t FileSystem::buscarBloqueInicioOrden(uint32_t idOrden) {
    uint32_t actual = superbloque.punteroIndiceOrdenes;
    char buffer[TAMANO_BLOQUE];
    while (actual != FIN_CADENA) {
        leerBloque(actual, buffer);
        BloqueIndiceOrdenes indice;
        indice.deserializar(buffer);
        for (auto& e : indice.entradas) {
            if (e.idOrden == idOrden) return e.bloqueInicio;
        }
        actual = indice.siguienteBloque;
    }
    return FIN_CADENA;
}

void FileSystem::registrarOrdenEnIndice(uint32_t idOrden, uint32_t bloqueInicio, uint32_t timestamp) {
    uint32_t actual = superbloque.punteroIndiceOrdenes;
    char buffer[TAMANO_BLOQUE];

    while (true) {
        leerBloque(actual, buffer);
        BloqueIndiceOrdenes indice;
        indice.deserializar(buffer);

        if ((int)indice.entradas.size() < MAX_ENTRADAS_INDICE_ORD) {
            EntradaIndiceOrden nueva{idOrden, bloqueInicio, timestamp};
            indice.entradas.push_back(nueva);
            indice.serializar(buffer);
            escribirBloque(actual, buffer);
            return;
        }

        if (indice.siguienteBloque == FIN_CADENA) {
            uint32_t nuevoBloque = asignarBloque();
            indice.siguienteBloque = nuevoBloque;
            indice.serializar(buffer);
            escribirBloque(actual, buffer);

            BloqueIndiceOrdenes vacio;
            vacio.serializar(buffer);
            escribirBloque(nuevoBloque, buffer);
        }
        actual = indice.siguienteBloque;
    }
}

void FileSystem::crearOrden(uint32_t idOrden, const std::vector<ItemOrden>& items) {
    char buffer[TAMANO_BLOQUE];
    uint32_t primerBloque = asignarBloque();
    uint32_t bloqueActual = primerBloque;

    size_t inicio = 0;
    while (inicio < items.size()) {
        BloqueOrden bloque;
        bloque.idOrden = idOrden;
        size_t fin = std::min(inicio + (size_t)MAX_ITEMS_POR_BLOQUE, items.size());
        for (size_t i = inicio; i < fin; i++) bloque.items.push_back(items[i]);
        inicio = fin;

        if (inicio < items.size()) {
            // Quedan mas items: la orden crece encadenando otro bloque
            uint32_t siguienteBloque = asignarBloque();
            bloque.siguienteBloque = siguienteBloque;
            bloque.serializar(buffer);
            escribirBloque(bloqueActual, buffer);
            bloqueActual = siguienteBloque;
        } else {
            bloque.serializar(buffer);
            escribirBloque(bloqueActual, buffer);
        }
    }

    registrarOrdenEnIndice(idOrden, primerBloque, (uint32_t)std::time(nullptr));
}

std::vector<ItemOrden> FileSystem::leerOrden(uint32_t idOrden, bool* encontrada) {
    std::vector<ItemOrden> resultado;
    uint32_t actual = buscarBloqueInicioOrden(idOrden);
    if (encontrada) *encontrada = (actual != FIN_CADENA);

    char buffer[TAMANO_BLOQUE];
    while (actual != FIN_CADENA) {
        leerBloque(actual, buffer);
        BloqueOrden bloque;
        bloque.deserializar(buffer);
        for (auto& it : bloque.items) resultado.push_back(it);
        actual = bloque.siguienteBloque;
    }
    return resultado;
}

void FileSystem::imprimirEstadoBitmap() {
    char buffer[TAMANO_BLOQUE];
    leerBloque(BLOQUE_BITMAP, buffer);
    std::cout << "Bloques ocupados: ";
    for (int b = 0; b < 40; b++) { // solo mostramos los primeros 40 para no llenar la pantalla
        bool ocupado = buffer[b / 8] & (1 << (b % 8));
        std::cout << (ocupado ? '1' : '0');
    }
    std::cout << " ..." << std::endl;
}