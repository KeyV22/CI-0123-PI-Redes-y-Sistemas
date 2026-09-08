# TicAmazon — Modelo de Almacenamiento en Disco

## Introducción

Este documento describe el sistema de almacenamiento diseñado e implementado para el catálogo de productos (bodegas) y las órdenes de compra del sistema **TicAmazon**.

El sistema utiliza:

- Bloques de tamaño fijo de **256 bytes**.
- Un **superbloque** que centraliza los metadatos generales.
- Un **bitmap** que controla a nivel de bit qué bloques están libres u ocupados.
- Un **directorio principal** para ubicar rápidamente cada bodega o categoría.
- Un mecanismo de **encadenamiento mediante punteros** que permite que las estructuras crezcan dinámicamente.

La implementación se realizó en **C++**, mediante la clase `FileSystem`, con serialización y deserialización manual de las estructuras a offsets exactos dentro de cada bloque. El funcionamiento fue probado incluyendo la persistencia real de los datos entre ejecuciones.

## 1. Firma del Sistema de Archivos

Todo archivo comienza con una firma de identificación de **4 bytes (`KEY1`)**, ubicada en los primeros 4 bytes del superbloque (bloque 0).

La firma:

- Se escribe al crear el archivo.
- Se verifica cada vez que el programa abre el archivo.
- Si no coincide con `KEY1`, el archivo se rechaza por considerarse corrupto o incompatible.

## 2. Superbloque — Bloque 0

El superbloque contiene los metadatos generales que describen la estructura completa del archivo.

| Campo | Offset | Tamaño | Descripción |
|---|---:|---:|---|
| `firma` | 0–3 | 4 B | Identifica el archivo como válido (`KEY1`) |
| `tamanoBloque` | 4–5 | 2 B | Tamaño de cada bloque: 256 bytes |
| `cantidadBloques` | 6–9 | 4 B | Total de bloques: 2048 |
| `punteroBitmap` | 10–13 | 4 B | Bloque donde inicia el bitmap: 1 |
| `punteroIndiceCategorias` | 14–17 | 4 B | Bloque donde inicia el directorio: 2 |
| `punteroIndiceOrdenes` | 18–21 | 4 B | Bloque donde inicia el índice de órdenes: 3 |
| `relleno` | 22–255 | 234 B | Espacio reservado para uso futuro |

## 3. Bitmap de Asignación — Bloque 1

El bitmap ocupa un bloque de **256 bytes**, es decir, **2048 bits**.

Cada bit representa el estado de un bloque:

- `1` → bloque ocupado.
- `0` → bloque libre.

Esto permite administrar hasta **2048 bloques**, equivalentes a **512 KB** de almacenamiento.

Antes de escribir información nueva, el sistema busca el primer bit libre, lo marca como ocupado y utiliza ese número de bloque.

Si no hay bloques disponibles, la operación falla con el error `DISCO_LLENO` sin sobrescribir información existente.

### Algoritmo de búsqueda

```text
para cada byte del bitmap:
    para cada bit del byte:
        si el bit está en 0:
            marcarlo en 1
            devolver el número del bloque
```

## 4. Directorio Principal — Índice de Categorías

El directorio principal funciona como el directorio raíz de un sistema de archivos convencional.

Permite encontrar una bodega o categoría por su nombre sin tener que recorrer todo el archivo.

Comienza en el **bloque 2**.

Cada entrada contiene:

| Campo | Tamaño | Descripción |
|---|---:|---|
| `idCategoria` | 1 B | Código numérico de la categoría |
| `nombreCategoria` | 16 B | Nombre de la categoría |
| `bloqueInicio` | 4 B | Bloque donde comienza la bodega |

Cada entrada ocupa **21 bytes**. Los últimos 4 bytes del bloque se reservan para `siguiente_bloque_indice`.

Por ello, caben **12 categorías por bloque**.

Cuando se registra una categoría adicional y el bloque actual está lleno, se asigna automáticamente un nuevo bloque y se encadena al anterior.

## 5. Almacenamiento de una Bodega

Los productos de una categoría se almacenan directamente dentro de los bloques de categoría a los que apunta el directorio.

Cada bloque de categoría contiene un encabezado y una lista de productos.

### Encabezado

| Campo | Tamaño | Descripción |
|---|---:|---|
| `idCategoria` | 1 B | Identifica la categoría |
| `siguienteBloque` | 4 B | Bloque donde continúa la bodega |
| `cantidadProductos` | 1 B | Productos almacenados en este bloque |

### Producto

| Campo | Tamaño | Descripción |
|---|---:|---|
| `idProducto` | 4 B | Identificador único |
| `precio` | 4 B | Precio en centavos |
| `cantidadDisponible` | 4 B | Existencias actuales |
| `nombre` | 20 B | Nombre del producto |

El encabezado ocupa **6 bytes** y cada producto **32 bytes**, por lo que caben **7 productos por bloque**.

Cuando una categoría supera los 7 productos, se asigna otro bloque y se conecta mediante `siguienteBloque`.

Los bloques de una misma bodega no necesitan estar físicamente contiguos.

### Ejemplo

```text
Bloque 10 (Alimentos)
    siguienteBloque = 42
    7 productos

        ↓

Bloque 42 (Alimentos)
    siguienteBloque = 0xFFFFFFFF
    2 productos
```

## 6. Índice de Órdenes

El índice de órdenes comienza en el **bloque 3**.

Contiene una entrada por cada orden registrada.

| Campo | Tamaño | Descripción |
|---|---:|---|
| `idOrden` | 4 B | Identificador único de la orden |
| `bloqueInicio` | 4 B | Bloque donde comienzan los datos |
| `timestamp` | 4 B | Fecha y hora de creación |

Cada entrada ocupa **12 bytes**, por lo que caben **21 órdenes por bloque**.

Cuando el bloque se llena, el índice se extiende mediante un nuevo bloque encadenado.

## 7. Bloques de Orden

Los bloques de orden almacenan los productos que un cliente agregó a una orden.

### Encabezado

| Campo | Tamaño | Descripción |
|---|---:|---|
| `idOrden` | 4 B | Identifica la orden |
| `siguienteBloque` | 4 B | Bloque donde continúa la orden |
| `cantidadProductos` | 2 B | Ítems almacenados en el bloque |

### Ítem de orden

| Campo | Tamaño | Descripción |
|---|---:|---|
| `idProducto` | 4 B | Producto solicitado |
| `cantidad` | 2 B | Unidades solicitadas |
| `precioUnitario` | 4 B | Precio al momento de la compra |

El encabezado ocupa **10 bytes** y cada ítem ocupa **10 bytes**, por lo que caben **24 productos distintos por bloque**.

El total de la orden no se almacena directamente. Se recalcula mediante:

```text
total = Σ(cantidad × precioUnitario)
```

Esto evita inconsistencias si algún bloque se modifica incorrectamente.

## 8. Mecanismo de Encadenamiento

Las cuatro estructuras principales utilizan el mismo mecanismo:

1. Directorio de categorías.
2. Bodega de cada categoría.
3. Índice de órdenes.
4. Bloques de orden.

Cada estructura posee un campo `siguienteBloque` que apunta al bloque donde continúa la estructura cuando el bloque actual se llena.

El valor:

```text
0xFFFFFFFF
```

representa el final de la cadena.

Los bloques encadenados no necesitan ser físicamente consecutivos. El orden lógico está determinado por los punteros.

Esto permite reutilizar cualquier bloque libre indicado por el bitmap.

## 9. Operaciones Principales

### 9.1 Crear el archivo

Si el archivo no existe:

1. Se crea vacío.
2. Se inicializa el superbloque.
3. Se establecen los punteros:
   - Bitmap → bloque 1.
   - Directorio de categorías → bloque 2.
   - Índice de órdenes → bloque 3.
4. Se marcan esos 4 bloques como ocupados.
5. Se inicializan los bloques de los índices.

### 9.2 Agregar un producto

1. Se busca la categoría en el directorio.
2. Si no existe:
   - Se asigna un nuevo bloque.
   - Se almacena el producto.
   - Se registra la categoría en el directorio.
3. Si el directorio está lleno, se crea y encadena otro bloque.
4. Si la categoría ya existe, se recorre su cadena buscando espacio.
5. Si todos los bloques están llenos, se agrega un nuevo bloque.

### 9.3 Listar productos

1. Se consulta el directorio.
2. Se obtiene el bloque inicial de la categoría.
3. Se recorre toda la cadena de bloques.
4. Se recopilan los productos encontrados.

### 9.4 Reservar existencias

1. Se recorre el directorio.
2. Se recorren las cadenas de cada categoría.
3. Se busca el producto mediante su identificador.
4. Si hay existencias suficientes, se descuenta la cantidad.
5. Se reescribe únicamente el bloque afectado.
6. Si no hay existencias suficientes, se genera un error sin modificar los datos.

### 9.5 Crear una orden

1. Se asignan los bloques necesarios.
2. Los bloques se encadenan si la orden supera los 24 ítems por bloque.
3. Se registra la orden en el índice.
4. Se guarda el bloque inicial y la fecha de creación.

### 9.6 Leer una orden

1. Se consulta el índice de órdenes.
2. Se obtiene el bloque inicial.
3. Se recorre toda la cadena.
4. Se recopilan los ítems.
5. Se recalcula el total de la compra.


---

## Resumen de la distribución del archivo

```text
┌──────────────────────────────────────┐
│ Bloque 0                             │
│ SUPERBLOQUE                          │
│ - Firma ORD1                         │
│ - Tamaño de bloque                   │
│ - Cantidad de bloques                │
│ - Puntero al bitmap                  │
│ - Puntero al índice de categorías    │
│ - Puntero al índice de órdenes       │
├──────────────────────────────────────┤
│ Bloque 1                             │
│ BITMAP                               │
├──────────────────────────────────────┤
│ Bloque 2                             │
│ ÍNDICE DE CATEGORÍAS                 │
│        ↓                             │
│ Bloques adicionales si es necesario  │
├──────────────────────────────────────┤
│ Bloque 3                             │
│ ÍNDICE DE ÓRDENES                    │
│        ↓                             │
│ Bloques adicionales si es necesario  │
├──────────────────────────────────────┤
│ Bloques libres / datos               │
│                                      │
│ Bodegas → bloques encadenados        │
│ Órdenes → bloques encadenados        │
└──────────────────────────────────────┘
```
