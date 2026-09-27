# TicAmazon - Tarea corta #11

## Estructura de carpetas

```
Almacenamiento/    -> almacenamiento.cpp/hpp, bloques.hpp (contenedor, 15%)
Cliente/            -> Cliente.cpp (cliente con la jerarquia de clases, 5%)
ServidorProductos/  -> ServidorProductos.cpp + VSocket/Socket/SSLSocket (50%)
Protocolo/           -> Protocolo.cpp/hpp (10%)
Simulacion/          -> SimulacionMancomunada.cpp (10%)
Packet/               -> laboratorio de Packet Tracer (no se compila)
(raiz)                -> Intermediario.cpp, Descubrimiento.cpp/hpp, Logger.cpp/hpp, Makefile, ci0123.pem, key0123.pem
```

## Compilar

```
make
```

Esto genera cuatro ejecutables en la raiz: `ServidorProductos.out`, `Intermediario.out`, `Cliente.out` y `SimulacionMancomunada.out`.

Tambien se puede compilar cada uno por separado:

```
make ServidorProductos.out
make Intermediario.out
make Cliente.out
make SimulacionMancomunada.out
```

Requiere `libssl-dev` instalado (para SSL). En Ubuntu/Debian:

```
sudo apt install libssl-dev
```

Importante: siempre correr `make` desde la raiz del proyecto (donde esta el `Makefile`), no desde dentro de una subcarpeta.

## Ejecutar

### 1. Bodega (ServidorProductos)

```
./ServidorProductos.out [puerto] [archivo] [puertoMulticast] [ssl]
```

Ejemplo normal:
```
./ServidorProductos.out 9091 almacenamiento.data 5000
```

Ejemplo con SSL:
```
./ServidorProductos.out 9091 almacenamiento.data 5000 ssl
```

### 2. Intermediario (servidor web, conecta Cliente <-> Bodega)

```
./Intermediario.out [id] [puertoHTTP] [puertoMulticast] [ssl] [bodega_ssl]
```

Ejemplo normal:
```
./Intermediario.out INT_04 8080 5000
```

Ejemplo con HTTPS hacia el cliente y SSL hacia la bodega:
```
./Intermediario.out INT_04 8443 5000 ssl bodega_ssl
```

Importante: levantar primero la Bodega y esperar unos segundos antes del Intermediario, para que el descubrimiento por multicast alcance a anunciarse.

### 3. Cliente (consola)

```
./Cliente.out [host] [puerto] [ssl] [idCliente]
```

Ejemplo normal:
```
./Cliente.out
```

Ejemplo con HTTPS:
```
./Cliente.out 127.0.0.1 8443 ssl
```

Tambien se puede usar un navegador normal en vez de `Cliente.out`, abriendo `http://127.0.0.1:8080/`.

### 4. Simulacion con hilos (protocolo + almacenamiento, sin sockets)

```
./SimulacionMancomunada.out [archivo]
```

Ejemplo:
```
./SimulacionMancomunada.out
```

No necesita levantar la Bodega ni el Intermediario: corre los tres roles (Cliente, Intermediario, Bodega) como hilos dentro del mismo programa.

## Limpiar

```
make clean
```

