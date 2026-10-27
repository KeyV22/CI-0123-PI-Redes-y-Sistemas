# TicAmazon – Pruebas de descubrimiento por broadcast (laboratorio)

Se necesitan **2 computadoras en la misma isla**:
- **Compu A**: corre el servidor de productos.
- **Compu B**: corre el intermediario.

Los comandos se copian tal cual. Solo hay que reemplazar `isla3` si se trabaja en otra isla
(`isla1` … `isla6`).

---

## Paso 1. Preparar las dos compus (A y B)

Ver la IP de la compu y su broadcast:
```bash
ip -4 addr show | grep -E "inet .* brd"
```
Debe verse una IP que empiece con `172.16.123.` (ejemplo isla 3: `brd 172.16.123.63`).
Si no aparece, la compu no está bien conectada a la isla.

Abrir el firewall:
```bash
sudo firewall-cmd --add-port=9090/udp --add-port=9089/udp --add-port=9091/tcp --add-port=8080/tcp
```

---

## Paso 2. Compilar (en A y en B)

Ir a la carpeta del proyecto (ajustar la ruta si es distinta):
```bash
cd ~/Documentos/CI-0123-PI-Redes-y-Sistemas/"Tranajos en clase"/TC18/"Programacion de elementos"
```

Compilar los tres programas:
```bash
cd ServidorProductos && make clean && make && cd ..
```
```bash
cd Intermediario && make clean && make && cd ..
```
```bash
cd Descubrimiento && make clean && make && cd ..
```

Comprobar que existen los ejecutables:
```bash
ls ServidorProductos/servidor Intermediario/intermediario Descubrimiento/buscar Descubrimiento/responder
```

---

## Prueba 1. Programas de demostración

**En la compu A:**
```bash
cd Descubrimiento
./responder
```
Debe decir: `Hilo esperando solicitudes UDP en el puerto 9090`.

**En la compu B:**
```bash
cd Descubrimiento
./buscar isla3 9090 5
```
Resultado correcto en B:
```
Respuesta de 172.16.123.XX:9090 -> OFFER/Servidor-A/9091
1 respuesta(s) en 5 s.
```
En A aparece `Solicitud de ...` y `Respondido: ...`.
Para cerrar `responder` en A: presionar **Enter**.

---

## Prueba 2. El intermediario descubre al servidor

**En la compu A (servidor):**
```bash
cd ServidorProductos
./servidor 9091 bodegas.data 9090
```

**En la compu B (intermediario):**
```bash
cd Intermediario
./intermediario 8080 auto isla3 9090 9089
```
Resultado correcto en B:
```
Servidor de productos descubierto: 172.16.123.XX:9091
```
En A aparece: `Descubrimiento: ... -> OFFER/PRODUCTOS/9091`.

Dejar el intermediario corriendo en B para la prueba 3.

---

## Prueba 3. El servidor descubre al intermediario

**En la compu A:** cerrar el servidor con `Ctrl+C` y volver a iniciarlo así:
```bash
./servidor 9091 bodegas.data 9090 isla3 9089
```
Resultado correcto en A:
```
Descubrimiento: intermediario encontrado en 172.16.123.XX:8080
```
(Si el intermediario arranca después, el servidor reintenta cada 5 s hasta encontrarlo.)

---

## Prueba 4 (opcional). Probar el sistema completo con el cliente

**En cualquier compu**, usando la IP de la compu B:
```bash
cd ClienteServidor && make clean && make
./cliente IP_DE_B 8080
```
(Reemplazar `IP_DE_B` por la IP real, por ejemplo `172.16.123.52`.)

---

## Si algo falla

| Qué se ve | Qué hacer |
|---|---|
| `0 respuesta(s)` | Revisar con `ip -4 addr show` que la IP sea `172.16.123.x`; repetir el comando del firewall en las dos compus |
| `No se encontro ningun servidor de productos` | Iniciar primero el servidor en A (Prueba 2) |
| `No se pudo iniciar el descubrimiento (UDP ...)` | Hay otro programa usando ese puerto: cerrarlo (`Ctrl+C`) y reintentar |
| El programa se comporta raro | `make clean && make` en esa carpeta |

---

## Antes de entregar

Limpiar archivos generados:
```bash
cd ServidorProductos && make clean && rm -f servidor.txt bodegas_prueba.data && cd ..
```
```bash
cd Intermediario && make clean && rm -f servidor.txt && cd ..
```
```bash
cd Descubrimiento && make clean && cd ..
```