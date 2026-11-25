# MODELO DE ALMACENAMIENTO - VERSION CON I-NODES

## 1. Descripcion general

- Todo el almacenamiento es **un unico archivo**, dividido en bloques de tamaño fijo de 256 bytes, numerados 0, 1, 2, ..., n-1.
- El bloque 0 es un **bloque de control** con informacion global del sistema.
- Existe una zona de **bloques de directorio**, encadenados, que listan las bodegas existentes y donde esta el **i-node** de cada una.
- Cada bodega tiene un **i-node**: un bloque que guarda la lista de punteros a los bloques de datos de esa bodega. Los bloques de datos ya **no se encadenan entre si**; es el i-node quien sabe cuales son y en que orden.
- El i-node tiene **10 punteros directos** a bloques de datos y **1 puntero indirecto** que apunta a un bloque de punteros. Si ese bloque de punteros se llena, se enlaza otro mediante su campo `bloque_siguiente`, formando una cadena de **bloques indirectos**.
- El directorio sigue siendo una cadena enlazada de bloques, con `bloque_siguiente` apuntando al proximo bloque de directorio, o -1 si es el ultimo.
- Tanto el directorio como cada bodega son **auto-extensibles**: cuando algo se llena, se obtiene un bloque libre (o se agrega uno nuevo al final del archivo) y se anota en la estructura correspondiente. No hay un limite fijo de bodegas ni de productos por bodega.
- Los bloques que no estan en uso forman una **lista de bloques libres**, para reciclarlos antes de hacer crecer el archivo.

## 2. Tipos de bloque

| Valor | Nombre | Descripcion |
|---|---|---|
| 0 | bloque libre | no esta en uso, disponible para asignar |
| 1 | bloque de directorio | contiene entradas (nombre de bodega + puntero a su i-node) |
| 2 | bloque de datos | contiene el texto de una bodega (sus productos) |
| 3 | bloque i-node | contiene los punteros a los bloques de datos de una bodega |
| 4 | bloque indirecto | contiene mas punteros a bloques de datos, cuando los directos no alcanzan |

El bloque 0 del archivo es distinto a todos estos: es el bloque de control (ver seccion 3), y no representa ninguno de estos tipos.

## 3. Bloque de control (bloque 0) - 256 bytes

| Campo | Tamaño (bytes) | Descripcion |
|---|---|---|
| num_bloques_totales | 4 | cantidad total de bloques en el archivo |
| primer_bloque_directorio | 4 | indice del primer bloque de directorio |
| primer_bloque_libre | 4 | indice del primer bloque libre de la lista de libres, -1 si no hay ninguno |
| cantidad_bloques_libres | 4 | cuantos bloques libres hay actualmente |
| cantidad_bodegas | 4 | cuantas bodegas existen actualmente |
| relleno | 236 | bytes sin usar (reservado) |

Suma total: 4+4+4+4+4+236 = 256 bytes

## 4. Encabezado comun a todos los demas bloques (12 bytes)

Todo bloque que no sea el bloque de control empieza con estos 3 campos:

| Campo | Tamaño (bytes) | Descripcion |
|---|---|---|
| tipo_bloque | 4 | 0 = libre, 1 = directorio, 2 = datos, 3 = i-node, 4 = indirecto |
| bloque_siguiente | 4 | indice del siguiente bloque encadenado, -1 si no hay mas |
| cantidad_usada | 4 | segun el tipo: entradas de directorio validas, bytes de texto usados, o punteros validos en un bloque indirecto (en el i-node no se usa, vale 0) |

## 5. Bloque tipo 1: directorio - 256 bytes

Encabezado comun (12 bytes) + una lista de entradas de directorio. Cada entrada:

| Campo | Tamaño (bytes) | Descripcion |
|---|---|---|
| nombre_bodega | 20 | nombre de la bodega |
| bloque_inodo | 4 | indice del bloque i-node de esa bodega |

Cada entrada ocupa 24 bytes. En el espacio disponible (256 - 12 = 244 bytes) caben 10 entradas (240 bytes), quedando 4 bytes de relleno.

Suma: 12 (comun) + 10×24 + 4 (relleno) = 256 bytes

Si se necesita una entrada de directorio numero 11 (o mas), el campo `bloque_siguiente` de este bloque apunta a otro bloque tipo 1, formando una cadena tan larga como se necesite.

## 6. Bloque tipo 3: i-node de una bodega - 256 bytes

| Campo | Tamaño (bytes) | Descripcion |
|---|---|---|
| (encabezado comun) | 12 | tipo_bloque = 3, bloque_siguiente = -1, cantidad_usada = 0 (estos dos ultimos no se usan en el i-node) |
| cantidad_bloques_datos | 4 | cuantos bloques de datos tiene la bodega en total (directos + indirectos) |
| directos[10] | 40 | indices de los primeros 10 bloques de datos, -1 en los que no se usan |
| indirecto | 4 | indice del primer bloque indirecto, -1 si todavia no hay ninguno |
| relleno | 196 | bytes sin usar (reservado) |

Suma: 12 + 4 + 40 + 4 + 196 = 256 bytes

Los punteros estan en orden: `directos[0]` es el primer bloque de datos de la bodega, `directos[1]` el segundo, y asi sucesivamente. Del bloque de datos numero 11 en adelante, los punteros se guardan en los bloques indirectos (seccion 7).

## 7. Bloque tipo 4: indirecto (bloque de punteros) - 256 bytes

| Campo | Tamaño (bytes) | Descripcion |
|---|---|---|
| (encabezado comun) | 12 | tipo_bloque = 4, bloque_siguiente = proximo bloque indirecto (o -1 si es el ultimo), cantidad_usada = cuantos punteros validos tiene |
| punteros[61] | 244 | indices de bloques de datos (4 bytes cada uno) |

Suma: 12 (comun) + 61×4 = 256 bytes

Un bloque indirecto guarda hasta 61 punteros. Cuando se llena, se pide otro bloque (libre o nuevo al final del archivo), se marca tipo 4 y se engancha con `bloque_siguiente`. Por eso una bodega puede crecer sin tope: el i-node apunta al primer indirecto, y los indirectos forman una cadena.

Orden de los punteros de una bodega: primero los 10 directos del i-node, luego los 61 del primer indirecto, luego los 61 del segundo, etc.
i-node ── directos[0..9] ──────────→ bloques de datos
│
└─ indirecto → [indirecto 1] ── 61 punteros → bloques de datos
│
└─ bloque_siguiente → [indirecto 2] ── 61 punteros → bloques de datos
│
└─ ... (sigue mientras haga falta)


## 8. Bloque tipo 2: datos de una bodega - 256 bytes

Encabezado comun (12 bytes) + un area de texto de 244 bytes, donde se van empacando registros de productos, uno detras de otro. Cada campo va separado por `:` y cada registro termina en salto de linea (`\n`):
nombre_bodega:categoria:producto:cantidad:precio

| Campo | Tamaño (bytes) | Descripcion |
|---|---|---|
| texto | 244 | registros de productos en texto, empacados uno tras otro mientras quepan completos |

Suma: 12 (comun) + 244 = 256 bytes

`cantidad_usada` (del encabezado comun) indica cuantos bytes del area de texto estan ocupados. Un registro nunca se parte a la mitad entre dos bloques: si el proximo registro no cabe completo en el espacio restante del ultimo bloque de datos, se obtiene un bloque nuevo y se anota su indice al final de la lista de punteros del i-node. En los bloques de datos `bloque_siguiente` no se usa (vale -1), porque el orden lo da el i-node.

## 9. Bloque tipo 0: libre - 256 bytes

| Campo | Tamaño (bytes) | Descripcion |
|---|---|---|
| (encabezado comun) | 12 | tipo_bloque = 0, bloque_siguiente apunta al proximo bloque libre (o -1 si es el ultimo) |
| sin_usar | 244 | contenido sin definir |

La lista de bloques libres es una lista enlazada: el bloque de control apunta al primero (`primer_bloque_libre`) y sabe cuantos hay en total (`cantidad_bloques_libres`); cada bloque libre apunta al siguiente usando su propio `bloque_siguiente`.

## 10. Como se agrega una bodega nueva

1. Se recorre la cadena de bloques de directorio buscando uno con espacio (menos de 10 entradas). Si ninguno tiene espacio, se obtiene un bloque libre (o se agrega uno nuevo al final del archivo si no hay libres), se marca tipo 1, y se engancha al final de la cadena de directorio con `bloque_siguiente`.
2. Se obtiene un bloque para el i-node de la bodega (reciclado de la lista de libres, o nuevo al final del archivo).
3. Se obtiene otro bloque para el primer bloque de datos, se marca tipo 2, y se pone `cantidad_usada = 0` (todavia sin productos).
4. Se escribe el i-node: tipo 3, `cantidad_bloques_datos = 1`, `directos[0]` = bloque de datos del paso 3, el resto de directos en -1 e `indirecto = -1`.
5. Se agrega una entrada nueva en el bloque de directorio con espacio: `nombre_bodega` y `bloque_inodo` apuntando al bloque del paso 2.
6. Se suma 1 a `cantidad_bodegas` en el bloque de control, y se resta del contador de libres cada bloque que se haya tomado de la lista de libres.

## 11. Como se agrega un producto a una bodega

1. Se busca la entrada de la bodega recorriendo la cadena de bloques de directorio, y se obtiene `bloque_inodo`.
2. Se lee el i-node y se reconstruye la lista de punteros a bloques de datos (los directos y luego los de la cadena de indirectos). El ultimo puntero es el ultimo bloque de datos de la bodega.
3. Se arma el texto del registro: `nombre_bodega:categoria:producto:cantidad:precio\n`
4. Si el registro completo cabe en el espacio restante del ultimo bloque de datos (244 - cantidad_usada), se agrega ahi y se actualiza `cantidad_usada`.
5. Si no cabe, se obtiene un bloque nuevo (reciclado o al final del archivo), se marca tipo 2, se escribe el registro ahi, y se agrega su indice al final de la lista de punteros del i-node:
   - Si hay menos de 10 bloques de datos, el indice va en el siguiente puntero directo.
   - Si no, va en el ultimo bloque indirecto. Si ese esta lleno (61 punteros), se obtiene un bloque nuevo, se marca tipo 4, y se engancha con `bloque_siguiente` al final de la cadena de indirectos. Si todavia no existia ningun indirecto, el i-node apunta al nuevo con su campo `indirecto`.
6. Se actualiza `cantidad_bloques_datos` en el i-node.

## 12. Como se extrae (elimina) un producto de una bodega

1. Se busca la bodega en el directorio y se obtiene su i-node.
2. Se recorren los bloques de datos en el orden de los punteros del i-node, buscando el primer registro cuyo nombre de producto coincida.
3. Al encontrarlo, se borra el registro corriendo hacia atras el resto del texto del bloque, y se resta su tamaño a `cantidad_usada`.
4. Si el bloque de datos quedo vacio y no es el primero de la bodega, se quita su puntero de la lista (los punteros siguientes se corren una posicion para no dejar huecos), se baja `cantidad_bloques_datos`, y el bloque se libera (tipo 0, al inicio de la lista de libres). Si al compactar sobra un bloque indirecto vacio, tambien se libera.
5. El primer bloque de datos nunca se libera aunque quede vacio, para que la bodega siempre tenga al menos uno.

## 13. Como se elimina una bodega

1. Se leen todos los punteros del i-node (directos e indirectos). Cada bloque de datos se marca tipo_bloque = 0 (libre) y se inserta al inicio de la lista de bloques libres, sumando 1 a `cantidad_bloques_libres` por cada uno.
2. Se liberan igualmente todos los bloques indirectos de la cadena, y por ultimo el propio bloque i-node.
3. Se quita la entrada correspondiente del bloque de directorio donde estaba (marcando el espacio como vacio).
4. Se resta 1 a `cantidad_bodegas` en el bloque de control.
5. (Opcional) Si al quitar la entrada un bloque de directorio queda completamente vacio y no es el unico de la cadena, tambien se puede liberar siguiendo la misma logica del paso 1.

## 14. Ejemplo

**Bloque de control (bloque 0):**
- num_bloques_totales = 7
- primer_bloque_directorio = 1
- primer_bloque_libre = -1
- cantidad_bloques_libres = 0
- cantidad_bodegas = 2

**Bloque 1 (tipo 1, directorio):**
- tipo_bloque = 1, bloque_siguiente = -1, cantidad_usada = 2
- entrada 1: nombre_bodega = "Bodega-1", bloque_inodo = 2
- entrada 2: nombre_bodega = "Bodega-2", bloque_inodo = 5

**Bloque 2 (tipo 3, i-node de Bodega-1):**
- tipo_bloque = 3, bloque_siguiente = -1, cantidad_usada = 0
- cantidad_bloques_datos = 2
- directos = [3, 4, -1, -1, -1, -1, -1, -1, -1, -1]
- indirecto = -1

**Bloque 3 (tipo 2, datos de Bodega-1):**
- tipo_bloque = 2, bloque_siguiente = -1, cantidad_usada = 215 (5 registros; el sexto no cabe completo en lo que queda)
- texto:
Bodega-1:Alimentos y bebidas:banano:100:10
Bodega-1:Alimentos y bebidas:manzana:10:300
Bodega-1:Alimentos y bebidas:naranja:40:50
Bodega-1:Salud y belleza:perfume:33:44
Bodega-1:Salud y belleza:jabon:12:25


**Bloque 4 (tipo 2, datos de Bodega-1, continuacion):**
- tipo_bloque = 2, bloque_siguiente = -1, cantidad_usada = 80
- texto:

Bodega-1:Salud y belleza:shampoo:33:44
Bodega-1:Isla1:shampoo:22:11


**Bloque 5 (tipo 3, i-node de Bodega-2):**
- tipo_bloque = 3, bloque_siguiente = -1, cantidad_usada = 0
- cantidad_bloques_datos = 1
- directos = [6, -1, -1, -1, -1, -1, -1, -1, -1, -1]
- indirecto = -1

**Bloque 6 (tipo 2, datos de Bodega-2):**
- tipo_bloque = 2, bloque_siguiente = -1, cantidad_usada = 0
- texto: (vacio, bodega recien creada)

**Ejemplo con bloque indirecto:** si Bodega-1 llegara a tener 12 bloques de datos, su i-node tendria `cantidad_bloques_datos = 12`, los 10 `directos` llenos e `indirecto = 20`, por ejemplo. El bloque 20 seria tipo 4, con `cantidad_usada = 2`, `bloque_siguiente = -1` y `punteros[0]`, `punteros[1]` apuntando a los bloques de datos 11 y 12.