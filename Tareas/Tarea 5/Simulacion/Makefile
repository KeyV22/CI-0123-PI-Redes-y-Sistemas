# TicAmazon - Simulacion del protocolo cliente <-> intermediario <-> productos

CXX := g++
CXXFLAGS := -g -Wall
TARGETS := simulacion.out

all: $(TARGETS)

simulacion.out: simulacion.cpp Buzon.cpp ServidorProductos.cpp ServidorIntermedio.cpp Buzon.hpp ServidorProductos.hpp ServidorIntermedio.hpp
	$(CXX) $(CXXFLAGS) simulacion.cpp Buzon.cpp ServidorProductos.cpp ServidorIntermedio.cpp -o $@

clean:
	@rm -f *.out