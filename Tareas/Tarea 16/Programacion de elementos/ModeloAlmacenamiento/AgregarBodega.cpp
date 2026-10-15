#include <iostream>
#include <fstream>
#include "Headers/almacenamiento.hpp"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " <archivo> <nombre_bodega>\n";
        return 1;
    }
    std::string archivo = argv[1];
    std::string nombreBodega = argv[2];

    Almacenamiento almacen;
    std::ifstream prueba(archivo);
    bool existe = prueba.good();
    prueba.close();

    bool abierto = existe ? almacen.abrir_archivo(archivo) : almacen.crear_archivo(archivo);
    if (!abierto) {
        std::cerr << "No se pudo abrir/crear " << archivo << "\n";
        return 1;
    }

    if (!almacen.crear_bodega(nombreBodega)) {
        almacen.cerrar_archivo();
        return 1;
    }

    std::cout << "Bodega '" << nombreBodega << "' agregada.\nBodegas: ";
    for (auto& b : almacen.listar_bodegas()) std::cout << b << " ";
    std::cout << "\n";

    almacen.cerrar_archivo();
    return 0;
}