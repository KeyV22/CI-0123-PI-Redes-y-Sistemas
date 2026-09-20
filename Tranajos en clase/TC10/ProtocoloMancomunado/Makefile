# TicAmazon - Protocolo mancomunado (Cliente / Intermediario / Bodega)

CXX := g++
CXXFLAGS := -g -Wall -std=c++11
LDFLAGS := -pthread
DEPS := Socket.h VSocket.h FileSystem.hpp Logger.hpp Protocolo.hpp Descubrimiento.hpp
TARGETS := ServidorProductos.out Intermediario.out Cliente.out SimulacionMancomunada.out test_protocolo.out test_descubrimiento.out

all: $(TARGETS)

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.cc $(DEPS)
	$(CXX) $(CXXFLAGS) -c $<

ServidorProductos.out: ServidorProductos.o VSocket.o Socket.o FileSystem.o Logger.o Protocolo.o Descubrimiento.o
	$(CXX) ServidorProductos.o VSocket.o Socket.o FileSystem.o Logger.o Protocolo.o Descubrimiento.o $(LDFLAGS) -o $@

Intermediario.out: Intermediario.o VSocket.o Socket.o Logger.o Protocolo.o Descubrimiento.o
	$(CXX) Intermediario.o VSocket.o Socket.o Logger.o Protocolo.o Descubrimiento.o $(LDFLAGS) -o $@

Cliente.out: Cliente.o VSocket.o Socket.o Logger.o Protocolo.o
	$(CXX) Cliente.o VSocket.o Socket.o Logger.o Protocolo.o $(LDFLAGS) -o $@

SimulacionMancomunada.out: SimulacionMancomunada.o FileSystem.o Logger.o Protocolo.o
	$(CXX) SimulacionMancomunada.o FileSystem.o Logger.o Protocolo.o $(LDFLAGS) -o $@

test_protocolo.out: test_protocolo.o Protocolo.o
	$(CXX) test_protocolo.o Protocolo.o -o $@

test_descubrimiento.out: test_descubrimiento.o Descubrimiento.o Protocolo.o
	$(CXX) test_descubrimiento.o Descubrimiento.o Protocolo.o $(LDFLAGS) -o $@

clean:
	rm -f *.o *.out *.log *.dat
