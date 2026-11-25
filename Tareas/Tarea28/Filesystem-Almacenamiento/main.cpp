#include <iostream>
#include <filesystem>
#include <string>
#include "almacenamiento.hpp"

struct ProductoReal {
    const char* categoria;
    const char* nombre;
    int precio; // colones
};

const ProductoReal CATALOGO[] = {
    // Alimentos y bebidas (15)
    {"Alimentos y bebidas", "arroz", 1500},
    {"Alimentos y bebidas", "frijoles", 1300},
    {"Alimentos y bebidas", "azucar", 1100},
    {"Alimentos y bebidas", "cafe", 3800},
    {"Alimentos y bebidas", "leche", 1050},
    {"Alimentos y bebidas", "huevos", 3200},
    {"Alimentos y bebidas", "pan", 1800},
    {"Alimentos y bebidas", "banano", 900},
    {"Alimentos y bebidas", "manzana", 300},
    {"Alimentos y bebidas", "naranja", 250},
    {"Alimentos y bebidas", "atun", 1250},
    {"Alimentos y bebidas", "aceite de oliva", 6500},
    {"Alimentos y bebidas", "pasta", 950},
    {"Alimentos y bebidas", "jugo de naranja", 1700},
    {"Alimentos y bebidas", "galletas", 1400},
    // Salud y belleza (12)
    {"Salud y belleza", "shampoo", 3500},
    {"Salud y belleza", "acondicionador", 3600},
    {"Salud y belleza", "jabon de manos", 1900},
    {"Salud y belleza", "crema dental", 1600},
    {"Salud y belleza", "cepillo de dientes", 1200},
    {"Salud y belleza", "desodorante", 2400},
    {"Salud y belleza", "perfume", 25000},
    {"Salud y belleza", "protector solar", 9800},
    {"Salud y belleza", "crema hidratante", 5500},
    {"Salud y belleza", "papel higienico", 3900},
    {"Salud y belleza", "alcohol en gel", 2100},
    {"Salud y belleza", "toallas humedas", 1750},
    // Hogar (12)
    {"Hogar", "detergente", 4800},
    {"Hogar", "suavizante", 3300},
    {"Hogar", "cloro", 1500},
    {"Hogar", "escoba", 3000},
    {"Hogar", "trapeador", 3500},
    {"Hogar", "bolsas de basura", 2200},
    {"Hogar", "esponjas", 1100},
    {"Hogar", "lavaplatos", 1900},
    {"Hogar", "sarten", 12500},
    {"Hogar", "olla", 18000},
    {"Hogar", "vaso de vidrio", 1300},
    {"Hogar", "toalla de bano", 7500},
    // Electronica (11)
    {"Electronica", "pantalla", 85000},
    {"Electronica", "teclado", 12000},
    {"Electronica", "mouse", 7500},
    {"Electronica", "audifonos", 15000},
    {"Electronica", "cargador USB", 6500},
    {"Electronica", "cable HDMI", 5000},
    {"Electronica", "baterias AA", 3200},
    {"Electronica", "bombillo LED", 2800},
    {"Electronica", "parlante bluetooth", 22000},
    {"Electronica", "memoria USB", 6000},
    {"Electronica", "router wifi", 28000},
};
const int NUM_PRODUCTOS = sizeof(CATALOGO) / sizeof(CATALOGO[0]); // 50

int main() {
    Almacenamiento almacen;
    if (!almacen.crear_archivo("bodegas.data")) return 1;

    const char* bodegas[] = {"Bodega-1", "Bodega-2"};

    for (int b = 0; b < 2; b++) {
        if (!almacen.crear_bodega(bodegas[b])) {
            std::cout << "Error creando " << bodegas[b] << "\n";
            return 1;
        }
        for (int i = 0; i < NUM_PRODUCTOS; i++) {
            // cantidades distintas por bodega para que no sean iguales
            int cantidad = (b == 0) ? 10 + (i * 7) % 90 : 5 + (i * 11) % 80;
            bool ok = almacen.insertar_producto(bodegas[b],CATALOGO[i].categoria,CATALOGO[i].nombre,std::to_string(cantidad),std::to_string(CATALOGO[i].precio));
            if (!ok) std::cout << "Fallo insertando " << CATALOGO[i].nombre<< " en " << bodegas[b] << "\n";
        }
    }

    // Verificar total de productos
    size_t total = 0;
    for (auto& nombre : almacen.listar_bodegas()) {
        size_t n = almacen.listar_productos(nombre).size();
        std::cout << nombre << ": " << n << " productos\n";
        total += n;
    }
    std::cout << "Total: " << total << " productos\n";

    // Verificar restriccion de 256 bytes/bloque
    almacen.cerrar_archivo();
    auto tam = std::filesystem::file_size("bodegas.data");
    std::cout << "Tamano archivo: " << tam << " bytes = " << tam / TAM_BLOQUE<< " bloques de " << TAM_BLOQUE << " bytes, residuo = " << tam % TAM_BLOQUE << "\n";
    std::cout << "sizeof(Bloque) = " << sizeof(Bloque) << "\n";

    // Verificar persistencia: reabrir y mostrar algunos productos reales
    almacen.abrir_archivo("bodegas.data");
    auto prods = almacen.listar_productos("Bodega-1");
    std::cout << "\nTras reabrir, Bodega-1 tiene " << prods.size() << " productos. Primeros 5:\n";
    for (size_t i = 0; i < 5 && i < prods.size(); i++)
        std::cout << "  " << prods[i].producto << " (" << prods[i].categoria<< ") cant=" << prods[i].cantidad << " precio=" << prods[i].precio << "\n";

    // Probar extraer
    almacen.extraer_producto("Bodega-1", "arroz");
    std::cout << "\nTras extraer 'arroz', Bodega-1 tiene "<< almacen.listar_productos("Bodega-1").size() << " productos\n";

    almacen.cerrar_archivo();
    return 0;
}