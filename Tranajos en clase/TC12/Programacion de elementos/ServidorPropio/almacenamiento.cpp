#include "almacenamiento.hpp"

// Sentinela para "no hay bloque siguiente" / "no hay bloque de datos".
// Se usa -1 (en vez de 0xFFFFFFFF) porque los campos son int32_t con signo:
// así basta comprobar "< 0" para saber si es válido.
static const int32_t BLOQUE_INVALIDO = -1;

//Lectura y escritura de bloques
void Almacenamiento::leer_bloque(int32_t indice, Bloque& bloque) {
    archivo.seekg(static_cast<std::streamoff>(indice) * TAM_BLOQUE, std::ios::beg);
    archivo.read(bloque.raw, TAM_BLOQUE);
}
void Almacenamiento::escribir_bloque(int32_t indice, const Bloque& bloque) {
    archivo.seekp(static_cast<std::streamoff>(indice) * TAM_BLOQUE, std::ios::beg);
    archivo.write(bloque.raw, TAM_BLOQUE);
    archivo.flush();
}
BloqueControl Almacenamiento::leer_control() {
    Bloque b;
    leer_bloque(0, b);
    return b.control;
}
void Almacenamiento::escribir_control(const BloqueControl& control) {
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

int32_t Almacenamiento::obtener_bloque_libre() {
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

void Almacenamiento::liberar_bloque(int32_t indice) {
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

 
// Crear / abrir / cerrar archivo
bool Almacenamiento::crear_archivo(const std::string& ruta) {
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
        dir.directorio.entradas[i].primer_bloque_datos = BLOQUE_INVALIDO;
    }

    escribir_bloque(1, dir);
    return true;
    //return abrir_archivo(ruta)
}

bool Almacenamiento::abrir_archivo(const std::string& ruta) {
    //Manejo de hilos
    std::lock_guard<std::mutex> lock(mtx);
    archivo.open(ruta, std::ios::in | std::ios::out | std::ios::binary);
    return archivo.is_open();
}

void Almacenamiento::cerrar_archivo() {
    //Manejo de hilos
    std::lock_guard<std::mutex> lock(mtx);
    if (archivo.is_open()) archivo.close();
}

// Directorio de bodegas libres
bool Almacenamiento::buscar_entrada_bodega(const std::string& nombre_bodega, int32_t& bloque_dir_out, int& pos_entrada_out, int32_t& primer_bloque_datos_out) {
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
                primer_bloque_datos_out = e.primer_bloque_datos;
                return true;
            }
        }
        indice = b.directorio.encabezado.bloque_siguiente;
    }
    return false;
}

bool Almacenamiento::crear_bodega(const std::string& nombre_bodega) {
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
                nuevo_bloque.directorio.entradas[i].primer_bloque_datos = BLOQUE_INVALIDO;
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
    int32_t bloque_datos = obtener_bloque_libre();
    Bloque datos;
    datos.datos.encabezado.tipo_bloque = TIPO_DATOS;
    datos.datos.encabezado.bloque_siguiente = BLOQUE_INVALIDO;
    datos.datos.encabezado.cantidad_usada = 0;
    std::memset(datos.datos.texto, 0, TAM_TEXTO_DATOS);
    escribir_bloque(bloque_datos, datos);

    // Volver a leer el bloque de directorio (por si obtener_bloque_libre
    // lo desplazó al crecer el archivo) y escribir la entrada
    leer_bloque(indice, b);
    std::memset(b.directorio.entradas[pos_libre].nombre_bodega, 0, TAM_NOMBRE_BODEGA);
    std::memcpy(b.directorio.entradas[pos_libre].nombre_bodega, nombre_bodega.data(), nombre_bodega.size());
    b.directorio.entradas[pos_libre].primer_bloque_datos = bloque_datos;
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

std::string Almacenamiento::armar_registro(const std::string& bodega, const std::string& categoria,
                                            const std::string& producto, const std::string& cantidad,
                                            const std::string& precio) {
    return bodega + ":" + categoria + ":" + producto + ":" + cantidad + ":" + precio;
}

ProductoTexto Almacenamiento::parsear_registro(const std::string& registro) {
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

bool Almacenamiento::insertar_producto(const std::string& nombre_bodega, const std::string& categoria,
                                        const std::string& producto, const std::string& cantidad,
                                        const std::string& precio) {
    std::lock_guard<std::mutex> lock(mtx);

    int32_t bloque_dir; int pos_entrada; int32_t bloque_datos;
    if (!buscar_entrada_bodega(nombre_bodega, bloque_dir, pos_entrada, bloque_datos)) {
        return false; // la bodega no existe
    }

    std::string registro = armar_registro(nombre_bodega, categoria, producto, cantidad, precio) + "\n";
    if (registro.size() > (size_t)TAM_TEXTO_DATOS) {
        return false; // un solo registro no puede exceder el bloque
    }

    // Caminar hasta el último bloque de datos de la cadena
    int32_t indice = bloque_datos;
    Bloque b;
    while (true) {
        leer_bloque(indice, b);
        if (b.datos.encabezado.bloque_siguiente == BLOQUE_INVALIDO) break;
        indice = b.datos.encabezado.bloque_siguiente;
    }

    int espacio_libre = TAM_TEXTO_DATOS - b.datos.encabezado.cantidad_usada;
    if ((int)registro.size() <= espacio_libre) {
        std::memcpy(b.datos.texto + b.datos.encabezado.cantidad_usada, registro.data(), registro.size());
        b.datos.encabezado.cantidad_usada += (int32_t)registro.size();
        escribir_bloque(indice, b);
        return true;
    }

    // No cabe: pedir un bloque nuevo y encadenarlo
    int32_t nuevo = obtener_bloque_libre();
    Bloque nuevo_bloque;
    nuevo_bloque.datos.encabezado.tipo_bloque = TIPO_DATOS;
    nuevo_bloque.datos.encabezado.bloque_siguiente = BLOQUE_INVALIDO;
    nuevo_bloque.datos.encabezado.cantidad_usada = (int32_t)registro.size();
    std::memset(nuevo_bloque.datos.texto, 0, TAM_TEXTO_DATOS);
    std::memcpy(nuevo_bloque.datos.texto, registro.data(), registro.size());
    escribir_bloque(nuevo, nuevo_bloque);

    // Releer el último bloque (por si el archivo creció) y enlazarlo
    leer_bloque(indice, b);
    b.datos.encabezado.bloque_siguiente = nuevo;
    escribir_bloque(indice, b);

    return true;
}

bool Almacenamiento::extraer_producto(const std::string& nombre_bodega, const std::string& nombre_producto) {
    std::lock_guard<std::mutex> lock(mtx);

    int32_t bloque_dir; int pos_entrada; int32_t bloque_datos;
    if (!buscar_entrada_bodega(nombre_bodega, bloque_dir, pos_entrada, bloque_datos)) {
        return false;
    }

    int32_t indice = bloque_datos;
    int32_t anterior = BLOQUE_INVALIDO;

    while (indice != BLOQUE_INVALIDO) {
        Bloque b;
        leer_bloque(indice, b);

        int32_t usado = b.datos.encabezado.cantidad_usada;
        int32_t pos = 0;
        while (pos < usado) {
            // localizar el próximo '\n' dentro de la zona usada
            int32_t fin_linea = pos;
            while (fin_linea < usado && b.datos.texto[fin_linea] != '\n') fin_linea++;
            if (fin_linea >= usado) break; // línea incompleta, no debería pasar

            std::string registro(b.datos.texto + pos, fin_linea - pos);
            ProductoTexto p = parsear_registro(registro);

            if (p.producto == nombre_producto) {
                int32_t largo_linea = fin_linea - pos + 1; // incluye '\n'
                int32_t resto = usado - fin_linea - 1;
                std::memmove(b.datos.texto + pos, b.datos.texto + fin_linea + 1, resto);
                std::memset(b.datos.texto + pos + resto, 0, largo_linea);
                b.datos.encabezado.cantidad_usada -= largo_linea;

                // Si el bloque quedó vacío y no es el primero de la cadena
                // de la bodega, se libera y se desengancha.
                if (b.datos.encabezado.cantidad_usada == 0 && indice != bloque_datos) {
                    Bloque bloque_anterior;
                    leer_bloque(anterior, bloque_anterior);
                    bloque_anterior.datos.encabezado.bloque_siguiente = b.datos.encabezado.bloque_siguiente;
                    escribir_bloque(anterior, bloque_anterior);
                    liberar_bloque(indice);
                } else {
                    escribir_bloque(indice, b);
                }
                return true;
            }

            pos = fin_linea + 1;
        }

        anterior = indice;
        indice = b.datos.encabezado.bloque_siguiente;
    }

    return false;
}

// ---------------------------------------------------------------------
// Consultas
// ---------------------------------------------------------------------

std::vector<std::string> Almacenamiento::listar_bodegas() {
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

std::vector<ProductoTexto> Almacenamiento::listar_productos(const std::string& nombre_bodega) {
    //Manejo de hilos
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<ProductoTexto> resultado;

    int32_t bloque_dir; int pos_entrada; int32_t bloque_datos;
    if (!buscar_entrada_bodega(nombre_bodega, bloque_dir, pos_entrada, bloque_datos)) {
        return resultado; // bodega inexistente -> lista vacía
    }

    int32_t indice = bloque_datos;
    while (indice != BLOQUE_INVALIDO) {
        Bloque b;
        leer_bloque(indice, b);

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

        indice = b.datos.encabezado.bloque_siguiente;
    }
    return resultado;
}