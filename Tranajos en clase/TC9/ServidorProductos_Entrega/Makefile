# TicAmazon - Servidor de Productos (bodega) + Intermediario + Cliente + GestionBodega
#

CXX := g++
CXXFLAGS := -g -Wall -std=c++11
LDFLAGS := -pthread
DEPS := Socket.h VSocket.h FileSystem.hpp Logger.hpp
TARGETS := ServidorProductos.out Intermediario.out Cliente.out GestionBodega.out

all: $(TARGETS)

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.cc $(DEPS)
	$(CXX) $(CXXFLAGS) -c $<

ServidorProductos.out: ServidorProductos.o VSocket.o Socket.o FileSystem.o Logger.o
	$(CXX) ServidorProductos.o VSocket.o Socket.o FileSystem.o Logger.o $(LDFLAGS) -o $@

Intermediario.out: Intermediario.o VSocket.o Socket.o Logger.o
	$(CXX) Intermediario.o VSocket.o Socket.o Logger.o $(LDFLAGS) -o $@

Cliente.out: Cliente.o VSocket.o Socket.o Logger.o
	$(CXX) Cliente.o VSocket.o Socket.o Logger.o $(LDFLAGS) -o $@

GestionBodega.out: GestionBodega.o FileSystem.o
	$(CXX) GestionBodega.o FileSystem.o -o $@

clean:
	rm -f *.o *.out *.log *.dat
