#include <iostream>
#include "Headers/almacenamiento.hpp"

void imprimir_bodega(Almacenamiento& almacen, const std::string& nombre) {
    std::cout << "Productos en " << nombre << ":\n";
    for (auto& p : almacen.listar_productos(nombre))
        std::cout << "  " << p.producto << " (" << p.categoria << ") cant=" << p.cantidad
                   << " precio=" << p.precio << "\n";
    std::cout << "\n";
}

int main() {
    Almacenamiento almacen;
    almacen.crear_archivo("bodegas.data");
    almacen.crear_bodega("Bodega-1");
    almacen.crear_bodega("Bodega-2");

    almacen.insertar_producto("Bodega-1", "Alimentos y bebidas", "banano", "100", "10");
    almacen.insertar_producto("Bodega-1", "Alimentos y bebidas", "manzana", "10", "300");
    almacen.insertar_producto("Bodega-1", "Salud y belleza", "perfume", "33", "44");

    almacen.insertar_producto("Bodega-2", "Salud y belleza", "shampoo", "22", "11");
    almacen.insertar_producto("Bodega-2", "Islas", "pantalla", "5", "150");

    std::cout << "Bodegas en el archivo:\n";
    for (auto& nombre : almacen.listar_bodegas())
        std::cout << "  " << nombre << "\n";
    std::cout << "\n";

    // leer las dos bodegas
    imprimir_bodega(almacen, "Bodega-1");
    imprimir_bodega(almacen, "Bodega-2");

    std::cout << "Extrayendo 'manzana' de Bodega-1...\n\n";
    almacen.extraer_producto("Bodega-1", "manzana");

    imprimir_bodega(almacen, "Bodega-1");
    imprimir_bodega(almacen, "Bodega-2");

    almacen.cerrar_archivo();
    return 0;
}