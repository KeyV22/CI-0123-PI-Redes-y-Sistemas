# TicAmazon — Protocolo mancomunado (Cliente / Intermediario / Bodega)

## Compilar

```bash
make clean
make
```

## Ejecutar el sistema completo (2 bodegas + intermediario + cliente)

Necesitás **cuatro terminales**.

**Terminal 1 — bodega 1** (Alimentos/Bloques):
```bash
./ServidorProductos.out 9091 bodega1.dat 1 5000
```

**Terminal 2 — bodega 2** (Vehiculos/Reposteria):
```bash
./ServidorProductos.out 9092 bodega2.dat 2 5000
```

**Terminal 3 — intermediario:**
```bash
./Intermediario.out INT_04 8080 5000
```

**Terminal 4 — cliente:**
```bash
./Cliente.out 127.0.0.1 8080 CLI_04
```

Menú del cliente:
```
1) Listar categorias
2) Ver productos de una categoria
3) Agregar producto al carrito
4) Pedir factura proforma
5) Salir
```

## Simulación (sin red, con hilos)

```bash
./SimulacionMancomunada.out
```

Corre sola, no necesita las otras terminales. Revisá
`bitacora_simulacion_mancomunada.log` para ver cada mensaje del protocolo.

## Pruebas unitarias

```bash
./test_protocolo.out
./test_descubrimiento.out
```

## Reiniciar desde cero

```bash
make clean
```
