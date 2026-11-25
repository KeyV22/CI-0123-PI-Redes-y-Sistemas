#pragma once
#include <cstdint>
#include <cstring>

const int TAM_BLOQUE = 256;

const int32_t TIPO_LIBRE = 0;
const int32_t TIPO_DIRECTORIO = 1;
const int32_t TIPO_DATOS = 2;
const int32_t TIPO_INODO = 3;
const int32_t TIPO_INDIRECTO = 4;

const int NUM_DIRECTOS = 10;
const int PUNTEROS_POR_INDIRECTO = 61;

const int MAX_ENTRADAS_DIR = 10;
const int TAM_NOMBRE_BODEGA = 20;
const int TAM_TEXTO_DATOS = 244;

#pragma pack(push, 1)

// Bloque 0 bloque de control
struct BloqueControl {
    int32_t num_bloques_totales;
    int32_t primer_bloque_directorio;
    int32_t primer_bloque_libre;
    int32_t cantidad_bloques_libres;
    int32_t cantidad_bodegas;
    char relleno[236];
};

// Encabezado comun a directorio, datos y libre
struct EncabezadoComun {
    int32_t tipo_bloque;
    int32_t bloque_siguiente;
    int32_t cantidad_usada; // entradas validas o bytes usados 
};

struct EntradaDirectorio {
    char nombre_bodega[TAM_NOMBRE_BODEGA];
    int32_t bloque_inodo;
};

// Bloque 1 directorio
struct BloqueDirectorio {
    EncabezadoComun encabezado;
    EntradaDirectorio entradas[MAX_ENTRADAS_DIR];
    char relleno[4];
};

// Bloque 2 datos de una bodega
struct BloqueDatos {
    EncabezadoComun encabezado;
    char texto[TAM_TEXTO_DATOS];
};

// Bloque tipo 0: libre
struct BloqueLibre {
    EncabezadoComun encabezado;
    char sin_usar[TAM_TEXTO_DATOS];
};

// Bloque 3 i-node de una bodega
struct BloqueInodo {
    EncabezadoComun encabezado;                 // 12
    int32_t cantidad_bloques_datos;             // 4  cuantos bloques de datos tiene la bodega
    int32_t directos[NUM_DIRECTOS];             // 40 punteros directos a bloques de datos
    int32_t indirecto;                          // 4  primer bloque indirecto (-1 si no hay)
    char relleno[196];                          // 196
};

// Bloque 4bloque de punteros (indirecto)
struct BloqueIndirecto {
    EncabezadoComun encabezado;                 // bloque_siguiente = otro indirecto, cantidad_usada = punteros validos
    int32_t punteros[PUNTEROS_POR_INDIRECTO];   // 244 bytes
};

// Un bloque generico
// leer/escribir sin importar de que tipo sean.
union Bloque {
    BloqueControl control;
    BloqueDirectorio directorio;
    BloqueDatos datos;
    BloqueLibre libre;
    BloqueInodo inodo;
    BloqueIndirecto indirecto;
    char raw[TAM_BLOQUE];

    Bloque() { std::memset(raw, 0, TAM_BLOQUE); }
};

#pragma pack(pop)

static_assert(sizeof(BloqueControl) == TAM_BLOQUE, "BloqueControl debe medir 256 bytes");
static_assert(sizeof(BloqueDirectorio) == TAM_BLOQUE, "BloqueDirectorio debe medir 256 bytes");
static_assert(sizeof(BloqueDatos) == TAM_BLOQUE, "BloqueDatos debe medir 256 bytes");
static_assert(sizeof(BloqueLibre) == TAM_BLOQUE, "BloqueLibre debe medir 256 bytes");
static_assert(sizeof(BloqueInodo) == TAM_BLOQUE, "BloqueInodo debe medir 256 bytes");
static_assert(sizeof(BloqueIndirecto) == TAM_BLOQUE, "BloqueIndirecto debe medir 256 bytes");