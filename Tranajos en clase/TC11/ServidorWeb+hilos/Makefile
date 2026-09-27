# TicAmazon - Intermediario web (HTML, hilos) + Bodega (protocolo mancomunado)

CXX := g++
CXXFLAGS := -g -Wall -std=c++11
LDFLAGS := -pthread
LDFLAGS_SSL := -pthread -lssl -lcrypto
DEPS := Socket.hpp VSocket.hpp SSLSocket.hpp almacenamiento.hpp bloques.hpp Logger.hpp Protocolo.hpp Descubrimiento.hpp
TARGETS := ServidorProductos.out Intermediario.out SimulacionMancomunada.out

all: $(TARGETS)

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $<

ServidorProductos.out: ServidorProductos.o VSocket.o Socket.o SSLSocket.o almacenamiento.o Logger.o Protocolo.o Descubrimiento.o
	$(CXX) ServidorProductos.o VSocket.o Socket.o SSLSocket.o almacenamiento.o Logger.o Protocolo.o Descubrimiento.o $(LDFLAGS_SSL) -o $@

Intermediario.out: Intermediario.o VSocket.o Socket.o SSLSocket.o Logger.o Protocolo.o Descubrimiento.o
	$(CXX) Intermediario.o VSocket.o Socket.o SSLSocket.o Logger.o Protocolo.o Descubrimiento.o $(LDFLAGS_SSL) -o $@

# Simulacion con hilos (sin sockets): Cliente/Intermediario/Bodega se comunican
# por buzones en memoria, pero hablan el protocolo mancomunado real y usan el
# mismo Almacenamiento que la Bodega real. No necesita Socket/SSLSocket/Logger/
# Descubrimiento porque no hay red de por medio.
SimulacionMancomunada.out: SimulacionMancomunada.o almacenamiento.o Protocolo.o
	$(CXX) SimulacionMancomunada.o almacenamiento.o Protocolo.o $(LDFLAGS) -o $@

clean:
	rm -f *.o *.out *.log *.dat
