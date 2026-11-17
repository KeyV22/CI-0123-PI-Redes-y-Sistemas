# Sistema de almacenamiento por bloques - Bodegas

Programa en C++ que implementa un sistema de archivos propio (un solo
archivo dividido en bloques de 256 bytes) para guardar bodegas y sus
productos, e insertar/extraer productos sobre el.

## Archivos

- `bloques.h` - estructuras de los bloques (control, directorio, datos, libre)
- `almacenamiento.h` / `almacenamiento.cpp` - logica para crear el archivo,
  crear bodegas, insertar y extraer productos
- `main.cpp` - programa de prueba: crea dos bodegas, inserta productos en
  cada una, las lista, y extrae un producto

## Compilar

```
make
```

Esto genera el ejecutable `bodegas`.

## Ejecutar

```
./bodegas
```

o directamente:

```
make run
```

Al ejecutarse, el programa crea el archivo `bodegas.data`
en el mismo directorio, y ahi guarda toda la informacion.

## Limpiar

```
make clean
```

Borra los archivos objeto, el ejecutable y `bodegas.data`.