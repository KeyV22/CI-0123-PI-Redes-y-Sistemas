// Verifica la regla de bloques de 256 bytes y la existencia de varias bodegas.
// Uso: ./verificar [archivo]
#include <iostream>
#include "Headers/almacenamiento.hpp"
#include "Headers/bloques.hpp"

static int fallos = 0;

static void chequear( const std::string & descripcion, bool condicion ) {
    std::cout << "[" << ( condicion ? "OK" : "FALLO" ) << "] " << descripcion << "\n";
    if ( !condicion ) fallos++;
}

int main( int argc, char * argv[] ) {
    std::string archivo = ( argc > 1 ) ? argv[1] : "verificacion.data";

    std::cout << "=== 1) Bloques de " << TAM_BLOQUE << " bytes ===\n";
    chequear( "BloqueControl mide 256 bytes",    sizeof( BloqueControl )    == (size_t)TAM_BLOQUE );
    chequear( "BloqueDirectorio mide 256 bytes", sizeof( BloqueDirectorio ) == (size_t)TAM_BLOQUE );
    chequear( "BloqueDatos mide 256 bytes",      sizeof( BloqueDatos )      == (size_t)TAM_BLOQUE );
    chequear( "BloqueLibre mide 256 bytes",      sizeof( BloqueLibre )      == (size_t)TAM_BLOQUE );

    std::cout << "\n=== 2) Varias bodegas ===\n";
    Almacenamiento almacen;
    chequear( "Se crea el archivo", almacen.crear_archivo( archivo ) );
    chequear( "Se crea Bodega-A", almacen.crear_bodega( "Bodega-A" ) );
    chequear( "Se crea Bodega-B", almacen.crear_bodega( "Bodega-B" ) );
    chequear( "Se crea Bodega-C", almacen.crear_bodega( "Bodega-C" ) );
    chequear( "No se puede duplicar una bodega", !almacen.crear_bodega( "Bodega-A" ) );
    chequear( "Hay 3 bodegas", almacen.listar_bodegas().size() == 3 );

    std::cout << "\n=== 3) Una bodega ocupa varios bloques de 256 bytes ===\n";
    const int N = 40;
    for ( int i = 0; i < N; i++ ) {
        almacen.insertar_producto( "Bodega-A", "CategoriaX", "Producto" + std::to_string( i ), "10", "1.50" );
    }
    chequear( "Se recuperan los 40 productos aunque pasen de un bloque",
              almacen.listar_productos( "Bodega-A" ).size() == (size_t)N );

    std::cout << "\n=== 4) Extraer una bodega no afecta a las demas ===\n";
    chequear( "Se extrae Bodega-B", almacen.extraer_bodega( "Bodega-B" ) );
    chequear( "Quedan 2 bodegas", almacen.listar_bodegas().size() == 2 );
    chequear( "Bodega-A conserva sus 40 productos", almacen.listar_productos( "Bodega-A" ).size() == (size_t)N );

    almacen.cerrar_archivo();
    std::cout << "\n" << ( fallos == 0 ? "Todas las verificaciones pasaron.\n" : "Hubo fallos.\n" );
    return fallos == 0 ? 0 : 1;
}