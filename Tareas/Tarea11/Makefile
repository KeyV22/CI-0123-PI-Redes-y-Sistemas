# TicAmazon - Tarea corta #11
#
# Cada punto de la rubrica vive en su propia carpeta, y cada ejecutable
# solo enlaza lo que realmente necesita:
#
#   ServidorProductos/  -> servidor de productos + jerarquia VSocket/Socket/SSLSocket (50%)
#   Cliente/             -> cliente que usa la jerarquia de clases (5%)
#                           (reusa el VSocket/Socket/SSLSocket de ServidorProductos/,
#                            no hay una segunda copia -- una sola jerarquia de clases)
#   Almacenamiento/      -> contenedor / modelo de almacenamiento propio (15%)
#   Protocolo/            -> protocolo de interaccion Intermediario<->Productos (10%)
#   Simulacion/           -> simulacion adaptada al protocolo + almacenamiento (10%)
#   Packet/               -> laboratorio de Packet Tracer (no es codigo, no se compila)
#   (raiz)                -> Intermediario.cpp, Descubrimiento.*, Logger.*, certificados
#                            (Intermediario no es un punto de la rubrica aparte, pero
#                            conecta Cliente<->Bodega)
#
# VPATH le dice a make en que carpetas buscar los .cpp/.hpp que no esten en
# la carpeta actual. CXXFLAGS con -I hace lo mismo para los #include de cada
# .cpp. Los .o siempre se generan en esta carpeta (raiz), asi que no importa
# de que subcarpeta haya salido cada fuente -- no hay choques de nombre
# porque cada archivo tiene un nombre unico.

CXX := g++
CXXFLAGS := -g -Wall -std=c++11 -IServidorProductos -IAlmacenamiento -IProtocolo -I.
LDFLAGS := -pthread
LDFLAGS_SSL := -pthread -lssl -lcrypto

VPATH := Almacenamiento:Cliente:ServidorProductos:Protocolo:Simulacion

DEPS := Socket.hpp VSocket.hpp SSLSocket.hpp almacenamiento.hpp bloques.hpp Logger.hpp Protocolo.hpp Descubrimiento.hpp
TARGETS := ServidorProductos.out Intermediario.out Cliente.out SimulacionMancomunada.out

all: $(TARGETS)

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $<

# --- Servidor de productos + jerarquia de clases VSocket/Socket/SSLSocket (50%) ---
# usa ademas el almacenamiento (15%), el protocolo (10%) y el descubrimiento multicast.
ServidorProductos.out: ServidorProductos.o VSocket.o Socket.o SSLSocket.o almacenamiento.o Logger.o Protocolo.o Descubrimiento.o
	$(CXX) ServidorProductos.o VSocket.o Socket.o SSLSocket.o almacenamiento.o Logger.o Protocolo.o Descubrimiento.o $(LDFLAGS_SSL) -o $@

# --- Intermediario (servidor web que conecta Cliente <-> Bodega) ---
Intermediario.out: Intermediario.o VSocket.o Socket.o SSLSocket.o Logger.o Protocolo.o Descubrimiento.o
	$(CXX) Intermediario.o VSocket.o Socket.o SSLSocket.o Logger.o Protocolo.o Descubrimiento.o $(LDFLAGS_SSL) -o $@

# --- Cliente que usa la jerarquia de clases e interactua con su propio servidor (5%) ---
# Reusa el mismo VSocket/Socket/SSLSocket de ServidorProductos/ (via VPATH), no
# habla el protocolo mancomunado directamente ni toca el almacenamiento.
Cliente.out: Cliente.o VSocket.o Socket.o SSLSocket.o
	$(CXX) Cliente.o VSocket.o Socket.o SSLSocket.o $(LDFLAGS_SSL) -o $@

# --- Simulacion adaptada al protocolo + almacenamiento (10%) ---
# Corre Cliente/Intermediario/Bodega como hilos en un solo programa, sin
# sockets (por eso no necesita VSocket/Socket/SSLSocket/Descubrimiento/Logger).
SimulacionMancomunada.out: SimulacionMancomunada.o almacenamiento.o Protocolo.o
	$(CXX) SimulacionMancomunada.o almacenamiento.o Protocolo.o $(LDFLAGS) -o $@

clean:
	rm -f *.o *.out *.log *.dat *.data
