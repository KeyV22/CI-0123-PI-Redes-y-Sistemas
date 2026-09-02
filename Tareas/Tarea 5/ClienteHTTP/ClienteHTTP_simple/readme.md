# TicAmazon - Cliente (version simple)

Version reducida a un solo archivo (`main.cpp`), sin separar en clases
`Cliente`/`Producto`/`Carrito`. Usa la misma jerarquia `VSocket`/`Socket`
para conectarse por HTTP y listar productos por categoria.

Diferencias con la version completa:
- Todo el flujo esta en `main.cpp` (sin encabezados propios ademas de Socket.h).
- No calcula `Content-Length`: simplemente lee hasta que el servidor cierra
  la conexion (por eso se manda `Connection: close` en la peticion).
- El carrito es un par de `vector` paralelos en vez de una clase.
- Sin manejo de excepciones para errores HTTP (codigo de estado, etc).

## Compilar y ejecutar

```bash
make
./cliente
```

## Limpiar

```bash
make clean
```
