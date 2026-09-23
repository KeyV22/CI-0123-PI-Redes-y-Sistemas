# TicAmazon - Intermediario web (HTML, hilos) + Bodega (protocolo mancomunado)

CXX := g++
CXXFLAGS := -g -Wall -std=c++11
LDFLAGS := -pthread
LDFLAGS_SSL := -pthread -lssl -lcrypto
DEPS := Socket.hpp VSocket.hpp SSLSocket.hpp almacenamiento.hpp bloques.hpp Logger.hpp Protocolo.hpp Descubrimiento.hpp
TARGETS := ServidorProductos.out Intermediario.out

all: $(TARGETS)

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $<

ServidorProductos.out: ServidorProductos.o VSocket.o Socket.o almacenamiento.o Logger.o Protocolo.o Descubrimiento.o
	$(CXX) ServidorProductos.o VSocket.o Socket.o almacenamiento.o Logger.o Protocolo.o Descubrimiento.o $(LDFLAGS) -o $@

Intermediario.out: Intermediario.o VSocket.o Socket.o SSLSocket.o Logger.o Protocolo.o Descubrimiento.o
	$(CXX) Intermediario.o VSocket.o Socket.o SSLSocket.o Logger.o Protocolo.o Descubrimiento.o $(LDFLAGS_SSL) -o $@

clean:
	rm -f *.o *.out *.log *.dat
