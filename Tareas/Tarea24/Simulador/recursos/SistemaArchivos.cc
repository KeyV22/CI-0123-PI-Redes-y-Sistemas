#include "SistemaArchivos.h"

// Sentinela para "no hay bloque siguiente" / "no hay bloque de datos".
// Se usa -1 (en vez de 0xFFFFFFFF) porque los campos son int32_t con signo:
// así basta comprobar "< 0" para saber si es válido.
static const int32_t BLOQUE_INVALIDO = -1;

//Lectura y escritura de bloques
void SistemaArchivos::leer_bloque(int32_t indice, Bloque& bloque) {
    archivo.seekg(static_cast<std::streamoff>(indice) * TAM_BLOQUE, std::ios::beg);
    archivo.read(bloque.raw, TAM_BLOQUE);
}
void SistemaArchivos::escribir_bloque(int32_t indice, const Bloque& bloque) {
    archivo.seekp(static_cast<std::streamoff>(indice) * TAM_BLOQUE, std::ios::beg);
    archivo.write(bloque.raw, TAM_BLOQUE);
    archivo.flush();
}
BloqueControl SistemaArchivos::leer_control() {
    Bloque b;
    leer_bloque(0, b);
    return b.control;
}
void SistemaArchivos::escribir_control(const BloqueControl& control) {
    Bloque b;
    b.control = control;
    escribir_bloque(0, b);
}


// Gestión de bloques libres
// ---------------------------------------------------------------------
// El archivo no tiene un tope fijo de bloques: si la lista de libres está
// vacía, simplemente se agrega un bloque nuevo al final del archivo. Así
// el almacenamiento crece bajo demanda en vez de estar limitado a los
// bloques reservados al crear el archivo.

int32_t SistemaArchivos::obtener_bloque_libre() {
    BloqueControl control = leer_control();

    if (control.primer_bloque_libre != BLOQUE_INVALIDO) {
        // Reutilizar el primero de la lista de libres
        int32_t indice = control.primer_bloque_libre;
        Bloque b;
        leer_bloque(indice, b);
        control.primer_bloque_libre = b.libre.encabezado.bloque_siguiente;
        control.cantidad_bloques_libres--;
        escribir_control(control);
        return indice;
    }

    // No hay bloques libres: extender el archivo con uno nuevo al final
    int32_t indice = control.num_bloques_totales;
    control.num_bloques_totales++;
    escribir_control(control);

    Bloque vacio; // el constructor de Bloque ya deja todo en cero
    escribir_bloque(indice, vacio);
    return indice;
}

void SistemaArchivos::liberar_bloque(int32_t indice) {
    BloqueControl control = leer_control();

    Bloque b;
    b.libre.encabezado.tipo_bloque = TIPO_LIBRE;
    b.libre.encabezado.bloque_siguiente = control.primer_bloque_libre;
    b.libre.encabezado.cantidad_usada = 0;
    escribir_bloque(indice, b);

    control.primer_bloque_libre = indice;
    control.cantidad_bloques_libres++;
    escribir_control(control);
}

std::vector<int32_t> SistemaArchivos::leer_punteros(int32_t indice_inodo) {
    std::vector<int32_t> v;
    Bloque bi;
    leer_bloque(indice_inodo, bi);
    int32_t n = bi.inodo.cantidad_bloques_datos;

    for (int i = 0; i < NUM_DIRECTOS && i < n; i++) {
        v.push_back(bi.inodo.directos[i]);
    }

    int32_t c = bi.inodo.indirecto;
    while (c != BLOQUE_INVALIDO && (int32_t)v.size() < n) {
        Bloque t;
        leer_bloque(c, t);
        for (int s = 0; s < t.indirecto.encabezado.cantidad_usada; s++) {
            v.push_back(t.indirecto.punteros[s]);
        }
        c = t.indirecto.encabezado.bloque_siguiente;
    }
    return v;
}

void SistemaArchivos::escribir_punteros(int32_t indice_inodo, const std::vector<int32_t>& v) {
    int32_t n = (int32_t)v.size();

    Bloque bi;
    leer_bloque(indice_inodo, bi);

    // Cadena de bloques indirectos que ya existe
    std::vector<int32_t> ind;
    int32_t c = bi.inodo.indirecto;
    while (c != BLOQUE_INVALIDO) {
        Bloque t;
        leer_bloque(c, t);
        ind.push_back(c);
        c = t.indirecto.encabezado.bloque_siguiente;
    }

    // Cuantos bloques indirectos hacen falta
    int32_t restantes = (n > NUM_DIRECTOS) ? (n - NUM_DIRECTOS) : 0;
    size_t necesarios = (restantes + PUNTEROS_POR_INDIRECTO - 1) / PUNTEROS_POR_INDIRECTO;

    while (ind.size() > necesarios) {          // liberarlos
        liberar_bloque(ind.back());
        ind.pop_back();
    }
    while (ind.size() < necesarios) {          // pedir libres o nuevos
        ind.push_back(obtener_bloque_libre());
    }

    // Escribir cada bloque indirecto
    for (size_t j = 0; j < ind.size(); j++) {
        Bloque t;
        t.indirecto.encabezado.tipo_bloque = TIPO_INDIRECTO;
        t.indirecto.encabezado.bloque_siguiente = (j + 1 < ind.size()) ? ind[j + 1] : BLOQUE_INVALIDO;

        int32_t inicio = NUM_DIRECTOS + (int32_t)j * PUNTEROS_POR_INDIRECTO;
        int32_t cant = n - inicio;
        if (cant > PUNTEROS_POR_INDIRECTO) cant = PUNTEROS_POR_INDIRECTO;
        t.indirecto.encabezado.cantidad_usada = cant;

        for (int s = 0; s < PUNTEROS_POR_INDIRECTO; s++) {
            t.indirecto.punteros[s] = (s < cant) ? v[inicio + s] : BLOQUE_INVALIDO;
        }
        escribir_bloque(ind[j], t);
    }

    // Escribir el i-node
    bi.inodo.cantidad_bloques_datos = n;
    for (int i = 0; i < NUM_DIRECTOS; i++) {
        bi.inodo.directos[i] = (i < n) ? v[i] : BLOQUE_INVALIDO;
    }
    bi.inodo.indirecto = ind.empty() ? BLOQUE_INVALIDO : ind[0];
    escribir_bloque(indice_inodo, bi);
}

 
// Crear / abrir / cerrar archivo
bool SistemaArchivos::crear_archivo(const std::string& ruta) {
    //Manejo de hilos con mutex
    std::lock_guard<std::mutex> lock(mtx);

    // std::ios::trunc crea el archivo si no existe, o lo vacía si existe
    archivo.open(ruta, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    if (!archivo.is_open()) return false;

    // Bloque 0: control. Arranca con solo el bloque de control (0) y el
    // primer bloque de directorio (1) ya reservados.
    BloqueControl control{};
    control.num_bloques_totales = 2;
    control.primer_bloque_directorio = 1;
    control.primer_bloque_libre = BLOQUE_INVALIDO;
    control.cantidad_bloques_libres = 0;
    control.cantidad_bodegas = 0;
    escribir_control(control);

    //Primer Bloque es directorio vacío
    Bloque dir;
    dir.directorio.encabezado.tipo_bloque = TIPO_DIRECTORIO;
    dir.directorio.encabezado.bloque_siguiente = BLOQUE_INVALIDO;
    dir.directorio.encabezado.cantidad_usada = 0;
    for (int i = 0; i < MAX_ENTRADAS_DIR; i++) {
        dir.directorio.entradas[i].nombre_bodega[0] = '\0';
        dir.directorio.entradas[i].bloque_inodo = BLOQUE_INVALIDO;
    }

    escribir_bloque(1, dir);
    return true;
    //return abrir_archivo(ruta)
}

bool SistemaArchivos::abrir_archivo(const std::string& ruta) {
    //Manejo de hilos
    std::lock_guard<std::mutex> lock(mtx);
    archivo.open(ruta, std::ios::in | std::ios::out | std::ios::binary);
    return archivo.is_open();
}

void SistemaArchivos::cerrar_archivo() {
    //Manejo de hilos
    std::lock_guard<std::mutex> lock(mtx);
    if (archivo.is_open()) archivo.close();
}

// Directorio de bodegas libres
bool SistemaArchivos::buscar_entrada_bodega(const std::string& nombre_bodega, int32_t& bloque_dir_out, int& pos_entrada_out, int32_t& bloque_inodo_out) {
    BloqueControl control = leer_control();
    int32_t indice = control.primer_bloque_directorio;

    while (indice != BLOQUE_INVALIDO) {
        Bloque b;
        leer_bloque(indice, b);
        for (int i = 0; i < MAX_ENTRADAS_DIR; i++) {
            const EntradaDirectorio& e = b.directorio.entradas[i];
            if (e.nombre_bodega[0] == '\0') continue;
            // nombre_bodega puede no venir terminado en '\0' si ocupa
            // exactamente TAM_NOMBRE_BODEGA caracteres.
            size_t len = strnlen(e.nombre_bodega, TAM_NOMBRE_BODEGA);
            if (len == nombre_bodega.size() &&
                std::memcmp(e.nombre_bodega, nombre_bodega.data(), len) == 0) {
                bloque_dir_out = indice;
                pos_entrada_out = i;
                bloque_inodo_out = e.bloque_inodo;
                return true;
            }
        }
        indice = b.directorio.encabezado.bloque_siguiente;
    }
    return false;
}

bool SistemaArchivos::crear_bodega(const std::string& nombre_bodega) {
    std::lock_guard<std::mutex> lock(mtx);

    if (nombre_bodega.empty() || nombre_bodega.size() >= (size_t)TAM_NOMBRE_BODEGA) {
        return false; // no cabe en el campo fijo
    }

    int32_t bloque_dir_existente; int pos_existente; int32_t datos_existente;
    if (buscar_entrada_bodega(nombre_bodega, bloque_dir_existente, pos_existente, datos_existente)) {
        return false; // ya existe
    }

    BloqueControl control = leer_control();

    // Recorrer la cadena de directorio buscando un espacio libre
    int32_t indice = control.primer_bloque_directorio;
    int32_t ultimo_indice = indice;
    Bloque b;
    int pos_libre = -1;

    while (true) {
        leer_bloque(indice, b);
        for (int i = 0; i < MAX_ENTRADAS_DIR; i++) {
            if (b.directorio.entradas[i].nombre_bodega[0] == '\0') {
                pos_libre = i;
                break;
            }
        }
        if (pos_libre != -1) break;

        ultimo_indice = indice;
        if (b.directorio.encabezado.bloque_siguiente == BLOQUE_INVALIDO) {
            // No hay espacio en ningún bloque de directorio existente:
            // encadenar uno nuevo.
            int32_t nuevo = obtener_bloque_libre();

            Bloque nuevo_bloque;
            nuevo_bloque.directorio.encabezado.tipo_bloque = TIPO_DIRECTORIO;
            nuevo_bloque.directorio.encabezado.bloque_siguiente = BLOQUE_INVALIDO;
            nuevo_bloque.directorio.encabezado.cantidad_usada = 0;
            for (int i = 0; i < MAX_ENTRADAS_DIR; i++) {
                nuevo_bloque.directorio.entradas[i].nombre_bodega[0] = '\0';
                nuevo_bloque.directorio.entradas[i].bloque_inodo = BLOQUE_INVALIDO;
            }
            escribir_bloque(nuevo, nuevo_bloque);

            // Enlazar el bloque anterior al nuevo (releer control por si
            // obtener_bloque_libre lo modificó)
            Bloque anterior;
            leer_bloque(ultimo_indice, anterior);
            anterior.directorio.encabezado.bloque_siguiente = nuevo;
            escribir_bloque(ultimo_indice, anterior);

            indice = nuevo;
            b = nuevo_bloque;
            pos_libre = 0;
            break;
        }
        indice = b.directorio.encabezado.bloque_siguiente;
    }

    // Reservar el primer bloque de datos de la bodega, vacío
    // Reservar el i-node de la bodega
    int32_t bloque_inodo = obtener_bloque_libre();

    // Reservar el primer bloque de datos de la bodega, vacío
    int32_t bloque_datos = obtener_bloque_libre();
    Bloque datos;
    datos.datos.encabezado.tipo_bloque = TIPO_DATOS;
    datos.datos.encabezado.bloque_siguiente = BLOQUE_INVALIDO;
    datos.datos.encabezado.cantidad_usada = 0;
    std::memset(datos.datos.texto, 0, TAM_TEXTO_DATOS);
    escribir_bloque(bloque_datos, datos);

    // Escribir el i-node apuntando a ese primer bloque de datos
    Bloque inodo;
    inodo.inodo.encabezado.tipo_bloque = TIPO_INODO;
    inodo.inodo.encabezado.bloque_siguiente = BLOQUE_INVALIDO;
    inodo.inodo.encabezado.cantidad_usada = 0;
    inodo.inodo.cantidad_bloques_datos = 1;
    for (int i = 0; i < NUM_DIRECTOS; i++) inodo.inodo.directos[i] = BLOQUE_INVALIDO;
    inodo.inodo.directos[0] = bloque_datos;
    inodo.inodo.indirecto = BLOQUE_INVALIDO;
    escribir_bloque(bloque_inodo, inodo);

    // Volver a leer el bloque de directorio y escribir la entrada
    leer_bloque(indice, b);
    std::memset(b.directorio.entradas[pos_libre].nombre_bodega, 0, TAM_NOMBRE_BODEGA);
    std::memcpy(b.directorio.entradas[pos_libre].nombre_bodega, nombre_bodega.data(), nombre_bodega.size());
    b.directorio.entradas[pos_libre].bloque_inodo = bloque_inodo;
    b.directorio.encabezado.cantidad_usada++;
    escribir_bloque(indice, b);

    control = leer_control();
    control.cantidad_bodegas++;
    escribir_control(control);

    return true;
}

// ---------------------------------------------------------------------
// Registros de producto (texto delimitado por ':' dentro del bloque)
// ---------------------------------------------------------------------

std::string SistemaArchivos::armar_registro(const std::string& bodega, const std::string& categoria,const std::string& producto, const std::string& cantidad,const std::string& precio) {
    return bodega + ":" + categoria + ":" + producto + ":" + cantidad + ":" + precio;
}

ProductoTexto SistemaArchivos::parsear_registro(const std::string& registro) {
    ProductoTexto p;
    std::string* campos[5] = {&p.bodega, &p.categoria, &p.producto, &p.cantidad, &p.precio};
    size_t inicio = 0;
    for (int i = 0; i < 5; i++) {
        size_t fin = (i < 4) ? registro.find(':', inicio) : registro.size();
        if (fin == std::string::npos) fin = registro.size();
        *campos[i] = registro.substr(inicio, fin - inicio);
        inicio = fin + 1;
    }
    return p;
}

// Insertar / extraer productos

bool SistemaArchivos::insertar_producto(const std::string& nombre_bodega, const std::string& categoria,const std::string& producto, const std::string& cantidad,const std::string& precio) {
    std::lock_guard<std::mutex> lock(mtx);
    int32_t bloque_dir; int pos_entrada; int32_t bloque_inodo;
    if (!buscar_entrada_bodega(nombre_bodega, bloque_dir, pos_entrada, bloque_inodo)) {
        return false; // la bodega no existe
    }

    std::string registro = armar_registro(nombre_bodega, categoria, producto, cantidad, precio) + "\n";
    if (registro.size() > (size_t)TAM_TEXTO_DATOS) {
        return false; // un solo registro no puede exceder el bloque
    }

    // El ultimo bloque de datos es el ultimo puntero del i-node
    std::vector<int32_t> punteros = leer_punteros(bloque_inodo);
    if (punteros.empty()) return false;
    int32_t indice = punteros.back();

    Bloque b;
    leer_bloque(indice, b);

    int espacio_libre = TAM_TEXTO_DATOS - b.datos.encabezado.cantidad_usada;
    if ((int)registro.size() <= espacio_libre) {
        std::memcpy(b.datos.texto + b.datos.encabezado.cantidad_usada, registro.data(), registro.size());
        b.datos.encabezado.cantidad_usada += (int32_t)registro.size();
        escribir_bloque(indice, b);
        return true;
    }

    // No cabe: pedir un bloque nuevo y anotarlo en el i-node
    int32_t nuevo = obtener_bloque_libre();
    Bloque nuevo_bloque;
    nuevo_bloque.datos.encabezado.tipo_bloque = TIPO_DATOS;
    nuevo_bloque.datos.encabezado.bloque_siguiente = BLOQUE_INVALIDO;
    nuevo_bloque.datos.encabezado.cantidad_usada = (int32_t)registro.size();
    std::memset(nuevo_bloque.datos.texto, 0, TAM_TEXTO_DATOS);
    std::memcpy(nuevo_bloque.datos.texto, registro.data(), registro.size());
    escribir_bloque(nuevo, nuevo_bloque);

    punteros.push_back(nuevo);
    escribir_punteros(bloque_inodo, punteros);

    return true;
}

bool SistemaArchivos::extraer_producto(const std::string& nombre_bodega, const std::string& nombre_producto) {
    std::lock_guard<std::mutex> lock(mtx);

    int32_t bloque_dir; int pos_entrada; int32_t bloque_inodo;
    if (!buscar_entrada_bodega(nombre_bodega, bloque_dir, pos_entrada, bloque_inodo)) {
        return false;
    }

    std::vector<int32_t> punteros = leer_punteros(bloque_inodo);

    for (size_t k = 0; k < punteros.size(); k++) {
        int32_t indice = punteros[k];
        Bloque b;
        leer_bloque(indice, b);

        int32_t usado = b.datos.encabezado.cantidad_usada;
        int32_t pos = 0;
        while (pos < usado) {
            int32_t fin_linea = pos;
            while (fin_linea < usado && b.datos.texto[fin_linea] != '\n') fin_linea++;
            if (fin_linea >= usado) break;

            std::string registro(b.datos.texto + pos, fin_linea - pos);
            ProductoTexto p = parsear_registro(registro);

            if (p.producto == nombre_producto) {
                int32_t largo_linea = fin_linea - pos + 1; // incluye '\n'
                int32_t resto = usado - fin_linea - 1;
                std::memmove(b.datos.texto + pos, b.datos.texto + fin_linea + 1, resto);
                std::memset(b.datos.texto + pos + resto, 0, largo_linea);
                b.datos.encabezado.cantidad_usada -= largo_linea;

                // Si el bloque quedo vacio y no es el primero de la bodega,
                // se quita del i-node (compactando punteros) y se libera.
                if (b.datos.encabezado.cantidad_usada == 0 && k != 0) {
                    punteros.erase(punteros.begin() + k);
                    escribir_punteros(bloque_inodo, punteros);
                    liberar_bloque(indice);
                } else {
                    escribir_bloque(indice, b);
                }
                return true;
            }

            pos = fin_linea + 1;
        }
    }

    return false;
}

// ---------------------------------------------------------------------
// Consultas
// ---------------------------------------------------------------------

std::vector<std::string> SistemaArchivos::listar_bodegas() {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::string> resultado;

    BloqueControl control = leer_control();
    int32_t indice = control.primer_bloque_directorio;

    while (indice != BLOQUE_INVALIDO) {
        Bloque b;
        leer_bloque(indice, b);
        for (int i = 0; i < MAX_ENTRADAS_DIR; i++) {
            const EntradaDirectorio& e = b.directorio.entradas[i];
            if (e.nombre_bodega[0] == '\0') continue;
            size_t len = strnlen(e.nombre_bodega, TAM_NOMBRE_BODEGA);
            resultado.emplace_back(e.nombre_bodega, len);
        }
        indice = b.directorio.encabezado.bloque_siguiente;
    }
    return resultado;
}

std::vector<ProductoTexto> SistemaArchivos::listar_productos(const std::string& nombre_bodega) {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<ProductoTexto> resultado;

    int32_t bloque_dir; int pos_entrada; int32_t bloque_inodo;
    if (!buscar_entrada_bodega(nombre_bodega, bloque_dir, pos_entrada, bloque_inodo)) {
        return resultado; // bodega inexistente -> lista vacia
    }

    std::vector<int32_t> punteros = leer_punteros(bloque_inodo);

    for (size_t k = 0; k < punteros.size(); k++) {
        Bloque b;
        leer_bloque(punteros[k], b);

        int32_t usado = b.datos.encabezado.cantidad_usada;
        int32_t pos = 0;
        while (pos < usado) {
            int32_t fin_linea = pos;
            while (fin_linea < usado && b.datos.texto[fin_linea] != '\n') fin_linea++;
            if (fin_linea >= usado) break;

            std::string registro(b.datos.texto + pos, fin_linea - pos);
            resultado.push_back(parsear_registro(registro));
            pos = fin_linea + 1;
        }
    }
    return resultado;
}