# TicAmazon - Servidor de productos

Servidor HTTP para la tienda virtual TicAmazon (Isla 3). Atiende solicitudes de
categorías, productos y proformas, y guarda su inventario en un contenedor
propio de bloques de 256 bytes (`bodegas.data`).

## Requisitos

- Linux (probado en Fedora/Ubuntu)
- g++ con soporte para C++17
- make


## Compilar

```bash
make
```

Genera el binario `servidor`.

## Limpiar

```bash
make clean
```

## Ejecutar

```bash
./servidor [puerto] [ruta_del_contenedor]
```

- `puerto`: por defecto `8080`.
- `ruta_del_contenedor`: por defecto `bodegas.data`.

También se puede ejecutar directamente con:

```bash
make run
make run PUERTO=9000
```

### Direccion IP

Para probar desde el lado del cliente puede hacerlo de forma local pero si lo prueba desde distintas computadoras en la misma red, es necesario que el cliente sepa la ip del servidor:

```bash
 ip addr show
```

Para detenerlo: `Ctrl+C`.

## Probar

Con `curl` (desde otra terminal, mientras el servidor sigue corriendo):

```bash
# Listar categorias
curl "http://<IP-del-servidor>:8080/TicAmazon/list.php"

# Listar productos de una categoria
curl "http://<IP-del-servidor>:8080/TicAmazon/list.php?category=Alimentos%20y%20bebidas"

# Generar una proforma
curl -X POST "http://<IP-del-servidor>:8080/TicAmazon/proforma.php" \
     -d "item0_categoria=Alimentos y bebidas&item0_producto=banano&item0_cantidad=3"
     
```

También se puede probar directo desde el navegador entrando a:

```bash
http://<IP-del-servidor>:8080/TicAmazon/list.php
```

## Otros comandos del Makefile

```bash
make objetos   # compila sin enlazar, solo para revisar errores rapido
make clean     # borra los binarios y objetos compilados
```
