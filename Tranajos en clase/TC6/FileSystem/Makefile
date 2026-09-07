# TicAmazon - Sistema de archivos (bloques de 256 bytes, directorio principal

CXX := g++
CXXFLAGS := -g -Wall -Wextra -std=c++17
TARGETS := demo

all: $(TARGETS)

demo: main.cpp FileSystem.cpp FileSystem.hpp
	$(CXX) $(CXXFLAGS) main.cpp FileSystem.cpp -o $@

run: demo
	./demo

clean:
	@rm -f demo *.dat *.o