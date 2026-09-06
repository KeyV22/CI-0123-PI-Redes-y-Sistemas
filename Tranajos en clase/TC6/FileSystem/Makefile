# TicAmazon - Sistema de archivos (bloques de 256 bytes, directorio principal

CXX := g++
CXXFLAGS := -g -Wall -Wextra -std=c++17
TARGETS := demo

all: $(TARGETS)

demo: main.cpp FileSystemTicAmazon.cpp FileSystemTicAmazon.hpp
	$(CXX) $(CXXFLAGS) main.cpp FileSystemTicAmazon.cpp -o $@

run: demo
	./demo

clean:
	@rm -f demo *.dat *.o