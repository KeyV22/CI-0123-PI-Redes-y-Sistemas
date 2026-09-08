# TicAmazon - Simulacion del protocolo cliente <-> intermediario <-> productos
# Version con hilos (std::thread) en vez de procesos (fork + IPC de kernel)

CXX := g++
CXXFLAGS := -g -Wall -std=c++17 -pthread
TARGETS := simulacion.out

all: $(TARGETS)

simulacion.out: simulacion.cpp Buzon.cpp ServidorProductos.cpp ServidorIntermedio.cpp Buzon.hpp ServidorProductos.hpp ServidorIntermedio.hpp
	$(CXX) $(CXXFLAGS) simulacion.cpp Buzon.cpp ServidorProductos.cpp ServidorIntermedio.cpp -o $@

clean:
	@rm -f *.out
