#ifndef FILESYSTEM_TICAMAZON_HPP
#define FILESYSTEM_TICAMAZON_HPP

#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

constexpr int TAMANO_BLOQUE = 256;
constexpr uint32_t FIN_CADENA = 0xFFFFFFFF;
const char FIRMA[5] = "KEY1";

//Bloques fijos
constexpr uint32_t BLOQUE_SUPERBLOQUE          = 0;
constexpr uint32_t BLOQUE_BITMAP               = 1;
constexpr uint32_t BLOQUE_INICIAL_CATEGORIAS   = 2;
constexpr uint32_t BLOQUE_INICIAL_ORDENES      = 3;

struct SuperBloque {
    char firma[4];
    uint16_t tamanoBloque;
    uint32_t cantidadBloques;
    uint32_t punteroBitmap;
    uint32_t punteroIndiceCategorias;
    uint32_t punteroIndiceOrdenes;

    void serializar(char* buffer) const;
    void deserializar(const char* buffer);
};

//Directorio principal (indice de categorias)
struct EntradaIndiceCategoria {
    uint8_t idCategoria;
    char nombreCategoria[16];
    uint32_t bloqueInicio; // primer bloque de la cadena BloqueCategoria de esta bodega
};

// Cuantas entradas caben en un bloque de indice, dejando 4 bytes al final
// para el puntero "siguiente_bloque_indice" (para cuando haya mas de
// MAX_ENTRADAS_INDICE_CAT categorias).
constexpr int TAM_ENTRADA_INDICE_CAT = 1 + 16 + 4; // 21 bytes
constexpr int MAX_ENTRADAS_INDICE_CAT = (TAMANO_BLOQUE - 4) / TAM_ENTRADA_INDICE_CAT; // 12

struct BloqueIndiceCategorias {
    std::vector<EntradaIndiceCategoria> entradas;
    uint32_t siguienteBloque = FIN_CADENA;

    void serializar(char* buffer) const;
    void deserializar(const char* buffer);
};

//Bloque de categoria (bodega): productos embebidos
struct Producto {
    uint32_t idProducto;
    uint32_t precio;            
    uint32_t cantidadDisponible;
    char nombre[20];
};

constexpr int TAM_PRODUCTO = 4 + 4 + 4 + 20; // 32 bytes
constexpr int TAM_ENCABEZADO_CAT = 1 + 4 + 1; // idCategoria + siguienteBloque + cantidadProductos = 6 bytes
constexpr int MAX_PRODUCTOS_POR_BLOQUE = (TAMANO_BLOQUE - TAM_ENCABEZADO_CAT) / TAM_PRODUCTO; // 7


struct BloqueCategoria {
    uint8_t idCategoria;
    uint32_t siguienteBloque = FIN_CADENA;
    std::vector<Producto> productos;

    void serializar(char* buffer) const;
    void deserializar(const char* buffer);
};

// Indice de ordenes 
struct EntradaIndiceOrden {
    uint32_t idOrden;
    uint32_t bloqueInicio;
    uint32_t timestamp;
};
constexpr int TAM_ENTRADA_INDICE_ORD = 4 + 4 + 4; // 12 bytes
constexpr int MAX_ENTRADAS_INDICE_ORD = (TAMANO_BLOQUE - 4) / TAM_ENTRADA_INDICE_ORD; // 21

struct BloqueIndiceOrdenes {
    std::vector<EntradaIndiceOrden> entradas;
    uint32_t siguienteBloque = FIN_CADENA;

    void serializar(char* buffer) const;
    void deserializar(const char* buffer);
};

//Bloque de orden: productos comprados 
struct ItemOrden {
    uint32_t idProducto;
    uint16_t cantidad;
    uint32_t precioUnitario;
};
constexpr int TAM_ITEM_ORDEN = 4 + 2 + 4; // 10 bytes
constexpr int TAM_ENCABEZADO_ORD = 4 + 4 + 2; // idOrden + siguienteBloque + cantidadProductos = 10 bytes
constexpr int MAX_ITEMS_POR_BLOQUE = (TAMANO_BLOQUE - TAM_ENCABEZADO_ORD) / TAM_ITEM_ORDEN; // 24

struct BloqueOrden {
    uint32_t idOrden;
    uint32_t siguienteBloque = FIN_CADENA;
    std::vector<ItemOrden> items;

    void serializar(char* buffer) const;
    void deserializar(const char* buffer);
};

class FileSystem {
    public:
        explicit FileSystem(const std::string& archivo);
        ~FileSystem();

        // --- Bodega / catalogo ---
        void agregarProducto(uint8_t idCategoria, const std::string& nombreCategoria, const Producto& producto);
        std::vector<Producto> listarProductos(uint8_t idCategoria);
        std::vector<std::string> listarCategorias();
        // true si encontro el producto y tenia stock suficiente (y lo descuenta)
        bool reservarProducto(uint32_t idProducto, uint32_t cantidad, std::string* motivoError = nullptr);

        // Ajusta el stock de un producto ya existente (delta positivo = reabastecer,
        // delta negativo = correccion de inventario). No permite dejar el stock en negativo.
        bool actualizarStock(uint32_t idProducto, int32_t delta, std::string* motivoError = nullptr);

        // Elimina por completo un producto del catalogo (lo "extrae" de la bodega),
        // liberando el espacio que ocupaba; si el bloque que lo contenia queda vacio
        // y no es el primero de la cadena de su categoria, tambien libera ese bloque.
        bool eliminarProducto(uint32_t idProducto, std::string* motivoError = nullptr);

        // --- Ordenes ---
        void crearOrden(uint32_t idOrden, const std::vector<ItemOrden>& items);
        std::vector<ItemOrden> leerOrden(uint32_t idOrden, bool* encontrada = nullptr);

        // --- Utilitario para depurar / mostrar la estructura interna ---
        void imprimirEstadoBitmap();

    private:
        std::fstream disco;
        SuperBloque superbloque;

        void inicializarArchivo();
        void leerBloque(uint32_t numeroBloque, char* buffer);
        void escribirBloque(uint32_t numeroBloque, const char* buffer);

        uint32_t asignarBloque();          // busca el primer bit libre en el bitmap, lo marca ocupado
        void liberarBloque(uint32_t numeroBloque);

        // Recorre el directorio principal (indice de categorias) buscando idCategoria.
        // Si existe, retorna su bloqueInicio; si no, retorna FIN_CADENA.
        uint32_t buscarBloqueInicioCategoria(uint8_t idCategoria);
        // Agrega una entrada nueva al indice de categorias (encadenando un bloque
        // de indice adicional si el ultimo esta lleno). Asi es como el
        // "directorio principal" crece.
        void registrarCategoriaEnIndice(uint8_t idCategoria, const std::string& nombre, uint32_t bloqueInicio);

        uint32_t buscarBloqueInicioOrden(uint32_t idOrden);
        void registrarOrdenEnIndice(uint32_t idOrden, uint32_t bloqueInicio, uint32_t timestamp);
};

#endif