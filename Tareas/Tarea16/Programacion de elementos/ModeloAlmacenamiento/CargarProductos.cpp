// Carga 2 bodegas con 50 productos cada una (100 en total) y lo verifica.
// Uso: ./cargar [archivo]
#include <iostream>
#include <string>
#include "Headers/almacenamiento.hpp"

int main( int argc, char * argv[] ) {
    std::string archivo = ( argc > 1 ) ? argv[1] : "bodegas100.data";

    // 5 categorias x 10 productos = 50 productos por bodega
    const char * categorias[] = { "Alimentos", "Bebidas", "Salud", "Electronica", "Hogar" };
    const char * bodegas[]    = { "Bodega-1", "Bodega-2" };

        // nombres[categoria][bodega][producto]
    const char * nombres[5][2][10] = {
        { // Alimentos
            { "Arroz", "Frijoles", "Azucar", "Sal", "Harina", "Aceite", "Pasta", "Atun", "Avena", "Cafe" },
            { "Lentejas", "Galletas", "Cereal", "Miel", "Mantequilla", "Salsa", "Sardinas", "Maiz", "Queso", "Jamon" }
        },
        { // Bebidas
            { "Agua", "Jugo-Naranja", "Refresco-Cola", "Te", "Leche", "Cerveza", "Limonada", "Energizante", "Soda", "Batido" },
            { "Jugo-Manzana", "Agua-Mineral", "Cafe-Frio", "Yogurt-Bebible", "Leche-Almendra", "Vino", "Horchata", "Gaseosa", "Kombucha", "Te-Helado" }
        },
        { // Salud
            { "Acetaminofen", "Ibuprofeno", "Alcohol", "Curitas", "Vitamina-C", "Mascarilla", "Termometro", "Jarabe", "Gasas", "Jabon-Antibacterial" },
            { "Aspirina", "Antiacido", "Suero", "Algodon", "Vitamina-D", "Guantes", "Crema-Solar", "Gotas-Ojos", "Vendas", "Gel-Antibacterial" }
        },
        { // Electronica
            { "Audifonos", "Cargador-USB", "Mouse", "Teclado", "Cable-HDMI", "Memoria-USB", "Parlante", "Bateria-AA", "Webcam", "Hub-USB" },
            { "Monitor", "Router", "Cable-Ethernet", "Adaptador-USB", "Power-Bank", "Microfono", "Disco-Externo", "Bateria-AAA", "Lampara-LED", "Soporte-Laptop" }
        },
        { // Hogar
            { "Escoba", "Trapeador", "Cubeta", "Esponja", "Detergente", "Bolsas-Basura", "Toalla", "Almohada", "Cobija", "Plato" },
            { "Vaso", "Cuchillo", "Olla", "Sarten", "Cortina", "Foco", "Extension", "Papel-Higienico", "Cesto", "Perchero" }
        }
    };
    Almacenamiento almacen;
    if ( !almacen.crear_archivo( archivo ) ) {
        std::cerr << "No se pudo crear " << archivo << "\n";
        return 1;
    }

    for ( int b = 0; b < 2; b++ ) {
        almacen.crear_bodega( bodegas[b] );
        for ( int c = 0; c < 5; c++ ) {
            for ( int p = 1; p <= 10; p++ ) {
                std::string producto = nombres[c][b][p - 1];
                std::string cantidad = std::to_string( 10 + p );
                std::string precio   = std::to_string( 100 * ( c + 1 ) + p ) + ".00";
                almacen.insertar_producto( bodegas[b], categorias[c], producto, cantidad, precio );
            }
        }
    }

    size_t total = 0;
    for ( const std::string & nombre : almacen.listar_bodegas() ) {
        size_t n = almacen.listar_productos( nombre ).size();
        std::cout << nombre << ": " << n << " productos\n";
        total += n;
    }
    std::cout << "Total: " << total << " productos en " << almacen.listar_bodegas().size() << " bodegas\n";
    std::cout << ( total == 100 ? "[OK] 100 productos en 2 bodegas\n" : "[FALLO] el total no es 100\n" );

    almacen.cerrar_archivo();
    return total == 100 ? 0 : 1;
}