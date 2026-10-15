CXX = g++
CXXFLAGS = -std=c++17 -Wall
PROGRAMAS = bodegas agregar_bodega extraer_bodega verificar cargar
HEADERS = Headers/almacenamiento.hpp Headers/bloques.hpp

all: $(PROGRAMAS)

bodegas: main.o almacenamiento.o
	$(CXX) $(CXXFLAGS) -o $@ $^

agregar_bodega: AgregarBodega.o almacenamiento.o
	$(CXX) $(CXXFLAGS) -o $@ $^

extraer_bodega: ExtraerBodega.o almacenamiento.o
	$(CXX) $(CXXFLAGS) -o $@ $^

verificar: VerificarSistemaArchivos.o almacenamiento.o
	$(CXX) $(CXXFLAGS) -o $@ $^

cargar: CargarProductos.o almacenamiento.o
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $<

almacenamiento.o: Code/almacenamiento.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.o $(PROGRAMAS) *.data