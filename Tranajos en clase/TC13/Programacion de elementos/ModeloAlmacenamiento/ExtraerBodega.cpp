#include <iostream>
#include "Headers/almacenamiento.hpp"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " <archivo> <nombre_bodega>\n";
        return 1;
    }
    std::string archivo = argv[1];
    std::string nombreBodega = argv[2];

    Almacenamiento almacen;
    if (!almacen.abrir_archivo(archivo)) {
        std::cerr << "No se pudo abrir " << archivo << " (debe existir)\n";
        return 1;
    }

    if (!almacen.extraer_bodega(nombreBodega)) {
        almacen.cerrar_archivo();
        return 1;
    }

    std::cout << "Bodega '" << nombreBodega << "' extraida.\nBodegas restantes: ";
    for (auto& b : almacen.listar_bodegas()) std::cout << b << " ";
    std::cout << "\n";

    almacen.cerrar_archivo();
    return 0;
}