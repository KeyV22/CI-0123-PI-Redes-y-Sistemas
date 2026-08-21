# CI-0123 — Proyecto Integrador de Redes y Sistemas Operativos
## Jerarquía de clases VSocket / Socket (versión Fedora)


## Contenido del proyecto

| Archivo | Descripción |
|---|---|
| `VSocket.h` / `VSocket.cc` | Clase base abstracta. Implementa `Init`, `Close`, `TryToConnect` (IPv4 e IPv6) y `Bind`. |
| `Socket.h` / `Socket.cc` | Clase derivada concreta. Implementa `Connect`, `Read`, `Write`, `sendTo`, `recvFrom`. |
| `ipv4-udp-server.cc` | Servidor UDP de prueba (plantilla). |
| `ipv4-udp-client.cc` | Cliente UDP de prueba (plantilla). |
| `Makefile` | Reglas de compilación para todos los binarios. |

---

## Requisitos

- Linux (probado en Fedora/Ubuntu)
- `g++` con soporte C++11 o superior
- `make`

---

## Compilación

Compilar todo el proyecto:

```bash
make clean
make ipv4-udp-server.out ipv4-udp-client.out
```

---


## 1) Prueba UDP sobre IPv4 (`ipv4-udp-server.out` + `ipv4-udp-client.out`)

Intercambio de un datagrama UDP cliente → servidor → cliente. Requiere **dos terminales**.

### En el laboratorio (contra el servidor real)

**Terminal 1 — levantar el servidor propio:**
```bash
./ipv4-udp-server.out
```
Queda esperando un datagrama en el puerto `1234` (bloqueado en `recvFrom`).

**Terminal 2 — correr el cliente:**
```bash
./ipv4-udp-client.out
```
El cliente está configurado para enviar su mensaje a `10.1.35.50:1234` (IP fija del servidor Python del laboratorio 3-5). Solo funciona conectado a esa red.

### Probar fuera del laboratorio

1. Editá `ipv4-udp-client.cc` y cambiá temporalmente:
   ```cpp
   n = inet_pton( AF_INET, "10.1.35.50", &other.sin_addr );
   ```
   por:
   ```cpp
   n = inet_pton( AF_INET, "127.0.0.1", &other.sin_addr );
   ```
2. Recompilá:
   ```bash
   make clean
   make ipv4-udp-server.out ipv4-udp-client.out
   ```
3. Terminal 1:
   ```bash
   ./ipv4-udp-server.out
   ```
4. Terminal 2:
   ```bash
   ./ipv4-udp-client.out
   ```

**Salida esperada, terminal del cliente:**
```
Client: Hello message sent.
Client message received: Hello 2026-ii from CI0123 server
```

**Salida esperada, terminal del servidor** (después de correr el cliente):
```
Server: message received: Hello 2026-ii from CI0123 client
Server: Hello message sent.
```





---


