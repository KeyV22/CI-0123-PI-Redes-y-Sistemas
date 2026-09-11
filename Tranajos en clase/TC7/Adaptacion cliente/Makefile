# TicAmazon - Servidor de productos (HTTP real) + Cliente
#

CXX := g++
CXXFLAGS := -g -Wall -std=c++11
LDFLAGS := -pthread
DEPS := Socket.h VSocket.h FileSystem.hpp Logger.hpp
TARGETS := ServidorProductos.out Cliente.out

all: $(TARGETS)

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.cc $(DEPS)
	$(CXX) $(CXXFLAGS) -c $<

ServidorProductos.out: ServidorProductos.o VSocket.o Socket.o FileSystem.o Logger.o
	$(CXX) ServidorProductos.o VSocket.o Socket.o FileSystem.o Logger.o $(LDFLAGS) -o $@

Cliente.out: Cliente.o VSocket.o Socket.o Logger.o
	$(CXX) Cliente.o VSocket.o Socket.o Logger.o $(LDFLAGS) -o $@

clean:
	rm -f *.o *.out *.log *.dat
