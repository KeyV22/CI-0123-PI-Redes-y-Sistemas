# Servidores TCP (Fork / Hilos) + Cliente

## Compilar

```bash
make clean
make
```

Esto genera `ForkMirrorServer.out`, `ThreadMirrorServer.out` y `MirrorClient.out`.

## Ejecutar

Necesitás **dos terminales**.

**Terminal 1 — servidor** (usá uno de los dos):
```bash
./ForkMirrorServer.out
```
```bash
./ThreadMirrorServer.out
```

**Terminal 2 — cliente:**
```bash
./MirrorClient.out "tu mensaje"
```

Si todo funciona, el cliente imprime de vuelta el mismo mensaje que mandó.
