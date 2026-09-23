#pragma once
#include <string>
#include <fstream>
#include <vector>
#include "bloques.hpp"

struct ProductoTexto {
    std::string bodega;
    std::string categoria;
    std::string producto;
    std::string cantidad;
    std::string precio;
};

class Almacenamiento {
public:
    // Crea el archivo desde cero bloque de control y un bloque de directorio vacio
    bool crear_archivo(const std::string& ruta);

    // Abre un archivo para trabajar con el
    bool abrir_archivo(const std::string& ruta);

    void cerrar_archivo();

    // Crea una bodega nueva le agrega su entrada en el directorioy le reserva su primer bloque de datos, vacio
    bool crear_bodega(const std::string& nombre_bodega);

    // Inserta un producto en la cadena de datos de una bodega
    bool insertar_producto(const std::string& nombre_bodega,const std::string& categoria,const std::string& producto,const std::string& cantidad,const std::string& precio);

    // Busca y elimina el primer producto que coincida con nombre_bodega mas producto
    bool extraer_producto(const std::string& nombre_bodega,const std::string& nombre_producto);

    // Ajusta la cantidad disponible de un producto ya existente (delta positivo
    // = reabastecer, negativo = reservar/vender). No deja la cantidad en negativo.
    // Reusa extraer_producto + insertar_producto (quita el registro viejo y mete
    // uno nuevo con la cantidad actualizada, conservando categoria y precio).
    bool actualizar_cantidad(const std::string& nombre_bodega,const std::string& nombre_producto,int delta,std::string* motivo_error = nullptr);

    // Utilidades para ver el estado del archivo
    std::vector<std::string> listar_bodegas();
    std::vector<ProductoTexto> listar_productos(const std::string& nombre_bodega);

private:
    std::fstream archivo;

    void leer_bloque(int32_t indice, Bloque& bloque);
    void escribir_bloque(int32_t indice, const Bloque& bloque);

    BloqueControl leer_control();
    void escribir_control(const BloqueControl& control);

    // Devuelve el indice de un bloque disponible
    int32_t obtener_bloque_libre();

    // Marca un bloque como libre y lo mete al inicio de la lista de libres
    void liberar_bloque(int32_t indice);

    // Busca la entrada de una bodega en el directorio
    bool buscar_entrada_bodega(const std::string& nombre_bodega,int32_t& bloque_dir_out,int& pos_entrada_out,int32_t& primer_bloque_datos_out);

    static std::string armar_registro(const std::string& bodega,const std::string& categoria,const std::string& producto,const std::string& cantidad,const std::string& precio);

    static ProductoTexto parsear_registro(const std::string& registro);
};