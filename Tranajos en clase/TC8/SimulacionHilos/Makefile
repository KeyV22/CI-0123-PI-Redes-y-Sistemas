# TicAmazon - Simulacion del protocolo con hilos (Cliente/Intermediario/ServidorProductos)

CXX := g++
CXXFLAGS := -g -Wall -std=c++11
LDFLAGS := -pthread
TARGETS := SimulacionProtocolo.out

all: $(TARGETS)

SimulacionProtocolo.out: SimulacionProtocolo.cpp FileSystem.cpp FileSystem.hpp Logger.cpp Logger.hpp
	$(CXX) $(CXXFLAGS) SimulacionProtocolo.cpp FileSystem.cpp Logger.cpp $(LDFLAGS) -o $@

clean:
	rm -f *.o *.out *.log *.dat
