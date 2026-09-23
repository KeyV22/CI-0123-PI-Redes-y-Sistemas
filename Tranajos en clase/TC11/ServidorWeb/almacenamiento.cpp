// almacenamiento.cpp
#include "almacenamiento.hpp"
#include <iostream>
#include <sstream>

// lectura y escritura de los bloques

void Almacenamiento::leer_bloque(int32_t indice, Bloque& bloque) {
    archivo.seekg((std::streamoff)indice * TAM_BLOQUE, std::ios::beg);
    archivo.read(bloque.raw, TAM_BLOQUE);
}

void Almacenamiento::escribir_bloque(int32_t indice, const Bloque& bloque) {
    archivo.seekp((std::streamoff)indice * TAM_BLOQUE, std::ios::beg);
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

//crear, abrir y cerrar
bool Almacenamiento::crear_archivo(const std::string& ruta) {
    archivo.open(ruta, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!archivo.is_open()) return false;

    // control
    Bloque bloque_control;
    bloque_control.control.num_bloques_totales = 2; 
    bloque_control.control.primer_bloque_directorio = 1;
    bloque_control.control.primer_bloque_libre = -1;
    bloque_control.control.cantidad_bloques_libres = 0;
    bloque_control.control.cantidad_bodegas = 0;
    escribir_bloque(0, bloque_control);
//drectorio
    Bloque bloque_dir;
    bloque_dir.directorio.encabezado.tipo_bloque = TIPO_DIRECTORIO;
    bloque_dir.directorio.encabezado.bloque_siguiente = -1;
    bloque_dir.directorio.encabezado.cantidad_usada = 0;
    escribir_bloque(1, bloque_dir);

    archivo.close();
    return abrir_archivo(ruta);
}

bool Almacenamiento::abrir_archivo(const std::string& ruta) {
    archivo.open(ruta, std::ios::in | std::ios::out | std::ios::binary);
    return archivo.is_open();
}

void Almacenamiento::cerrar_archivo() {
    if (archivo.is_open()) archivo.close();
}

// bloques libres

int32_t Almacenamiento::obtener_bloque_libre() {
    BloqueControl control = leer_control();

    if (control.primer_bloque_libre != -1) {
        int32_t indice = control.primer_bloque_libre;
        Bloque b;
        leer_bloque(indice, b);
        control.primer_bloque_libre = b.libre.encabezado.bloque_siguiente;
        control.cantidad_bloques_libres--;
        escribir_control(control);
        return indice;
    }

    // no hay libres y se agrega uno nuevo al final del archivo
    int32_t nuevo_indice = control.num_bloques_totales;
    control.num_bloques_totales++;
    escribir_control(control);
    return nuevo_indice;
}

void Almacenamiento::liberar_bloque(int32_t indice) {
    BloqueControl control = leer_control();

    Bloque b; // se sobreescribe todo como bloque libre
    b.libre.encabezado.tipo_bloque = TIPO_LIBRE;
    b.libre.encabezado.bloque_siguiente = control.primer_bloque_libre;
    b.libre.encabezado.cantidad_usada = 0;
    escribir_bloque(indice, b);

    control.primer_bloque_libre = indice;
    control.cantidad_bloques_libres++;
    escribir_control(control);
}

// directorio y bodegas

bool Almacenamiento::buscar_entrada_bodega(const std::string& nombre_bodega,int32_t& bloque_dir_out,int& pos_entrada_out,int32_t& primer_bloque_datos_out) {
    BloqueControl control = leer_control();
    int32_t indice = control.primer_bloque_directorio;
    while (indice != -1) {
        Bloque b;
        leer_bloque(indice, b);
        for (int i = 0; i < b.directorio.encabezado.cantidad_usada; i++) {
            if (nombre_bodega == b.directorio.entradas[i].nombre_bodega) {
                bloque_dir_out = indice;
                pos_entrada_out = i;
                primer_bloque_datos_out = b.directorio.entradas[i].primer_bloque_datos;
                return true;
            }
        }
        indice = b.directorio.encabezado.bloque_siguiente;
    }
    return false;
}

bool Almacenamiento::crear_bodega(const std::string& nombre_bodega) {
    int32_t bloque_dir; int pos; int32_t primer_datos;
    if (buscar_entrada_bodega(nombre_bodega, bloque_dir, pos, primer_datos)) {
        std::cerr << "Ya existe una bodega con ese nombre\n";
        return false;
    }

    BloqueControl control = leer_control();

    // buscar un bloque de directorio con espacio
    int32_t indice_dir = control.primer_bloque_directorio;
    int32_t ultimo_dir = -1;
    Bloque b_dir;
    while (indice_dir != -1) {
        leer_bloque(indice_dir, b_dir);
        if (b_dir.directorio.encabezado.cantidad_usada < MAX_ENTRADAS_DIR) break;
        ultimo_dir = indice_dir;
        indice_dir = b_dir.directorio.encabezado.bloque_siguiente;
    }

    if (indice_dir == -1) {
        // no hay espacio en ningun bloque de directorioy se crea uno nuevo
        indice_dir = obtener_bloque_libre();
        Bloque nuevo;
        nuevo.directorio.encabezado.tipo_bloque = TIPO_DIRECTORIO;
        nuevo.directorio.encabezado.bloque_siguiente = -1;
        nuevo.directorio.encabezado.cantidad_usada = 0;
        escribir_bloque(indice_dir, nuevo);

        // enganchar al final de la cadena de directorio
        leer_bloque(ultimo_dir, b_dir);
        b_dir.directorio.encabezado.bloque_siguiente = indice_dir;
        escribir_bloque(ultimo_dir, b_dir);

        b_dir = nuevo;
    }

    // crear el bloque de datos vacio de la bodega
    int32_t bloque_datos = obtener_bloque_libre();
    Bloque b_datos;
    b_datos.datos.encabezado.tipo_bloque = TIPO_DATOS;
    b_datos.datos.encabezado.bloque_siguiente = -1;
    b_datos.datos.encabezado.cantidad_usada = 0;
    escribir_bloque(bloque_datos, b_datos);

    // agregar la entrada en el bloque de directorio
    int pos_nueva = b_dir.directorio.encabezado.cantidad_usada;
    std::memset(b_dir.directorio.entradas[pos_nueva].nombre_bodega, 0, TAM_NOMBRE_BODEGA);
    std::strncpy(b_dir.directorio.entradas[pos_nueva].nombre_bodega,nombre_bodega.c_str(), TAM_NOMBRE_BODEGA - 1);
    b_dir.directorio.entradas[pos_nueva].primer_bloque_datos = bloque_datos;
    b_dir.directorio.encabezado.cantidad_usada++;
    escribir_bloque(indice_dir, b_dir);

    control = leer_control();
    control.cantidad_bodegas++;
    escribir_control(control);

    return true;
}

// productos formato

std::string Almacenamiento::armar_registro(const std::string& bodega,const std::string& categoria,const std::string& producto,const std::string& cantidad,const std::string& precio) {
    // Se usa '\n' como terminador de registro (no ".") porque el precio puede
    // traer punto decimal (ej: "3.25"), lo que chocaria con un terminador ".".
    return bodega + "," + categoria + "," + producto + "," + cantidad + "," + precio + "\n";
}

ProductoTexto Almacenamiento::parsear_registro(const std::string& registro) {
    ProductoTexto p;
    std::stringstream ss(registro);
    std::getline(ss, p.bodega, ',');
    std::getline(ss, p.categoria, ',');
    std::getline(ss, p.producto, ',');
    std::getline(ss, p.cantidad, ',');
    std::getline(ss, p.precio, '\n'); // el ultimo campo termina en salto de linea
    return p;
}

bool Almacenamiento::insertar_producto(const std::string& nombre_bodega,const std::string& categoria,const std::string& producto,const std::string& cantidad,const std::string& precio) {
    int32_t bloque_dir; int pos; int32_t indice_datos;
    if (!buscar_entrada_bodega(nombre_bodega, bloque_dir, pos, indice_datos)) {
        std::cerr << "No existe la bodega " << nombre_bodega << "\n";
        return false;
    }

    std::string registro = armar_registro(nombre_bodega, categoria, producto, cantidad, precio);
    int espacio_necesario = (int)registro.size();

    // ir hasta el ultimo bloque de datos de la cadena
    Bloque b;
    leer_bloque(indice_datos, b);
    int32_t indice_actual = indice_datos;
    while (b.datos.encabezado.bloque_siguiente != -1) {
        indice_actual = b.datos.encabezado.bloque_siguiente;
        leer_bloque(indice_actual, b);
    }

    int espacio_disponible = TAM_TEXTO_DATOS - b.datos.encabezado.cantidad_usada;

    if (espacio_necesario > espacio_disponible) {
        // si no cabe se agarra un bloque de datos nuevo
        int32_t nuevo_indice = obtener_bloque_libre();
        b.datos.encabezado.bloque_siguiente = nuevo_indice;
        escribir_bloque(indice_actual, b);

        Bloque nuevo;
        nuevo.datos.encabezado.tipo_bloque = TIPO_DATOS;
        nuevo.datos.encabezado.bloque_siguiente = -1;
        nuevo.datos.encabezado.cantidad_usada = 0;
        indice_actual = nuevo_indice;
        b = nuevo;
    }

    std::memcpy(b.datos.texto + b.datos.encabezado.cantidad_usada,
                registro.c_str(), registro.size());
    b.datos.encabezado.cantidad_usada += (int32_t)registro.size();
    escribir_bloque(indice_actual, b);

    return true;
}

bool Almacenamiento::extraer_producto(const std::string& nombre_bodega,const std::string& nombre_producto) {
    int32_t bloque_dir; int pos; int32_t indice_datos;
    if (!buscar_entrada_bodega(nombre_bodega, bloque_dir, pos, indice_datos)) {
        std::cerr << "No existe la bodega " << nombre_bodega << "\n";
        return false;
    }

    int32_t indice_actual = indice_datos;
    while (indice_actual != -1) {
        Bloque b;
        leer_bloque(indice_actual, b);

        std::string texto(b.datos.texto, b.datos.encabezado.cantidad_usada);
        size_t inicio = 0;

        while (inicio < texto.size()) {
            size_t fin = texto.find('\n', inicio); // fin de este registro
            if (fin == std::string::npos) break;
            std::string registro = texto.substr(inicio, fin - inicio + 1);
            ProductoTexto p = parsear_registro(registro);

            if (p.producto == nombre_producto) {
                // se quita este registroy se corre el resto del texto hacia la izquierda
                texto.erase(inicio, registro.size());
                std::memset(b.datos.texto, 0, TAM_TEXTO_DATOS);
                std::memcpy(b.datos.texto, texto.c_str(), texto.size());
                b.datos.encabezado.cantidad_usada = (int32_t)texto.size();
                escribir_bloque(indice_actual, b);
                return true;
            }
            inicio = fin + 1;
        }

        indice_actual = b.datos.encabezado.bloque_siguiente;
    }

    std::cerr << "No se encontro el producto " << nombre_producto
              << " en la bodega " << nombre_bodega << "\n";
    return false;
}

bool Almacenamiento::actualizar_cantidad(const std::string& nombre_bodega,const std::string& nombre_producto,int delta,std::string* motivo_error) {
    // buscar el registro actual para conocer sus campos (categoria, precio, cantidad)
    for (auto& p : listar_productos(nombre_bodega)) {
        if (p.producto != nombre_producto) continue;

        int cantidad_actual = std::stoi(p.cantidad);
        int nueva_cantidad = cantidad_actual + delta;

        if (nueva_cantidad < 0) {
            if (motivo_error) *motivo_error = "STOCK_INSUFICIENTE";
            return false;
        }

        if (!extraer_producto(nombre_bodega, nombre_producto)) return false;

        return insertar_producto(nombre_bodega, p.categoria, p.producto,
                                  std::to_string(nueva_cantidad), p.precio);
    }

    if (motivo_error) *motivo_error = "404";
    return false;
}

// listar para probar

std::vector<std::string> Almacenamiento::listar_bodegas() {
    std::vector<std::string> resultado;
    BloqueControl control = leer_control();
    int32_t indice = control.primer_bloque_directorio;

    while (indice != -1) {
        Bloque b;
        leer_bloque(indice, b);
        for (int i = 0; i < b.directorio.encabezado.cantidad_usada; i++) {
            resultado.push_back(b.directorio.entradas[i].nombre_bodega);
        }
        indice = b.directorio.encabezado.bloque_siguiente;
    }
    return resultado;
}

std::vector<ProductoTexto> Almacenamiento::listar_productos(const std::string& nombre_bodega) {
    std::vector<ProductoTexto> resultado;
    int32_t bloque_dir; int pos; int32_t indice_datos;
    if (!buscar_entrada_bodega(nombre_bodega, bloque_dir, pos, indice_datos)) return resultado;

    int32_t indice_actual = indice_datos;
    while (indice_actual != -1) {
        Bloque b;
        leer_bloque(indice_actual, b);
        std::string texto(b.datos.texto, b.datos.encabezado.cantidad_usada);

        size_t inicio = 0;
        while (inicio < texto.size()) {
            size_t fin = texto.find('\n', inicio);
            if (fin == std::string::npos) break;
            resultado.push_back(parsear_registro(texto.substr(inicio, fin - inicio + 1)));
            inicio = fin + 1;
        }
        indice_actual = b.datos.encabezado.bloque_siguiente;
    }
    return resultado;
}