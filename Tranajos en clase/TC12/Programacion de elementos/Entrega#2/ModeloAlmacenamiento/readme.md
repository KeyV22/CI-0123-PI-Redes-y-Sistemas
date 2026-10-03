# TicAmazon - Modelo de Almacenamiento

Demostración del contenedor de almacenamiento en disco usado por el
servidor de productos. Todo el inventario se guarda en un único archivo
binario (`bodegas.data`), dividido en bloques de tamaño fijo de 256
bytes. Un bloque de control administra el archivo, un bloque de
directorio (encadenable) lista las bodegas existentes, y cada bodega
tiene su propia cadena de bloques de datos donde se guardan sus
productos como texto. El archivo crece bajo demanda (no hay límite fijo
de bodegas ni de productos), y los bloques liberados se reciclan antes
de extender el archivo.

## Requisitos

- Linux (probado en Fedora/Ubuntu)
- g++ con soporte para C++17
- make

## Compilar

```bash
make
```

Genera el binario `bodegas`.

## Limpiar
```bash
make clean
```

## Ejecutar

```bash
make run
```

o directamente:

```bash
./bodegas
```

Esto crea `bodegas.data`, siembra dos bodegas de ejemplo con productos,
lista su contenido, extrae un producto y vuelve a listar para mostrar el
resultado.

## Salida esperada

```
Bodegas en el archivo:
  Bodega-1
  Bodega-2

Productos en Bodega-1:
  banano (Alimentos y bebidas) cant=100 precio=10
  manzana (Alimentos y bebidas) cant=10 precio=300
  perfume (Salud y belleza) cant=33 precio=44

Productos en Bodega-2:
  shampoo (Salud y belleza) cant=22 precio=11
  pantalla (Islas) cant=5 precio=150

Extrayendo 'manzana' de Bodega-1...

Productos en Bodega-1:
  banano (Alimentos y bebidas) cant=100 precio=10
  perfume (Salud y belleza) cant=33 precio=44

Productos en Bodega-2:
  shampoo (Salud y belleza) cant=22 precio=11
  pantalla (Islas) cant=5 precio=150
```