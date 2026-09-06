#include "FileSystem.hpp"
#include <iostream>
#include <cstring>

Producto crearProducto(uint32_t id, const std::string& nombre, uint32_t precio, uint32_t stock) {
    Producto p{};
    p.idProducto = id;
    p.precio = precio;
    p.cantidadDisponible = stock;
    memset(p.nombre, 0, 20);
    strncpy(p.nombre, nombre.c_str(), 19);
    return p;
}

int main() {
    std::remove("bodega.dat"); // empezar limpio en cada corrida de la demo

    FileSystem fs("bodega.dat");

    std::cout << "== Cargando catalogo ==\n";
    // Categoria 5 = Alimentos: cargamos 9 productos a proposito (caben 7 por
    // bloque) para forzar que el directorio encadene un segundo bloque de
    // categoria y demostrar que la estructura SI puede crecer.
    fs.agregarProducto(5, "Alimentos", crearProducto(101, "Arroz Tio con entradas", 1500, 340));
    fs.agregarProducto(5, "Alimentos", crearProducto(102, "Frijoles negros", 900, 120));
    fs.agregarProducto(5, "Alimentos", crearProducto(103, "Numar", 2200, 60));
    fs.agregarProducto(5, "Alimentos", crearProducto(104, "Azucar Sabe mas", 700, 200));
    fs.agregarProducto(5, "Alimentos", crearProducto(105, "Sal salada", 400, 300));
    fs.agregarProducto(5, "Alimentos", crearProducto(106, "Cafe 1820", 1800, 90));
    fs.agregarProducto(5, "Alimentos", crearProducto(107, "Atun Suli", 950, 150));
    // --- a partir de aqui ya no cabe en el primer bloque (limite: 7) ---
    fs.agregarProducto(5, "Alimentos", crearProducto(108, "Leche tres Pinos", 850, 80));
    fs.agregarProducto(5, "Alimentos", crearProducto(109, "Pan Bimbo", 1200, 45));

    fs.agregarProducto(3, "SaludYBelleza", crearProducto(205, "Shampoo para pies", 3200, 85));

    std::cout << "\n== Categorias registradas en el directorio principal ==\n";
    for (auto& c : fs.listarCategorias()) std::cout << "- " << c << std::endl;

    std::cout << "\n== Productos de Alimentos (categoria 5) ==\n";
    for (auto& p : fs.listarProductos(5)) {
        std::cout << "  id=" << p.idProducto << " " << p.nombre
                  << " precio=" << p.precio << " stock=" << p.cantidadDisponible << std::endl;
    }
    std::cout << "(deberian salir los 9 productos, aunque esten repartidos en 2 bloques encadenados)\n";

    std::cout << "\n== Reservando stock (simula un AGREGAR:producto:cantidad) ==\n";
    std::string error;
    if (fs.reservarProducto(108, 5, &error)) {
        std::cout << "OK: se reservaron 5 unidades de Leche tres Pinos (producto del SEGUNDO bloque encadenado)\n";
    } else {
        std::cout << "ERROR:" << error << std::endl;
    }

    if (!fs.reservarProducto(103, 9999, &error)) {
        std::cout << "ERROR:" << error << " (esperado:Numar solo tiene 60 unidades)\n";
    }

    std::cout << "\n== Creando una orden con productos de ambos bloques ==\n";
    std::vector<ItemOrden> items = {
        {101, 2, 1500},
        {108, 1, 850},
    };
    fs.crearOrden(7, items);

    std::cout << "\n== Leyendo la orden 7 desde disco ==\n";
    bool encontrada;
    auto itemsLeidos = fs.leerOrden(7, &encontrada);
    long total = 0;
    for (auto& it : itemsLeidos) {
        std::cout << "  producto=" << it.idProducto << " cantidad=" << it.cantidad
                  << " precio=" << it.precioUnitario << std::endl;
        total += (long)it.cantidad * it.precioUnitario;
    }
    std::cout << "  total=" << total << " (esperado: 2*1500 + 1*850 = 3850)\n";

    std::cout << "\n== Estado del bitmap tras todas las operaciones ==\n";
    fs.imprimirEstadoBitmap();

    return 0;
}