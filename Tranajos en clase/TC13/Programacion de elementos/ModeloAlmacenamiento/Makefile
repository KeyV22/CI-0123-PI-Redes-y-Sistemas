CXX = g++
CXXFLAGS = -std=c++17 -Wall

OBJS = main.o almacenamiento.o
BIN = bodegas

OBJS_AGREGAR = AgregarBodega.o almacenamiento.o
OBJS_EXTRAER = ExtraerBodega.o almacenamiento.o
OBJS_VERIFICAR = VerificarSistemaArchivos.o almacenamiento.o
OBJS_CARGAR = CargarProductos.o almacenamiento.o

all: $(BIN) agregar_bodega extraer_bodega verificar cargar

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(OBJS)

cargar: $(OBJS_CARGAR)
	$(CXX) $(CXXFLAGS) -o cargar $(OBJS_CARGAR)

CargarProductos.o: CargarProductos.cpp Headers/almacenamiento.hpp Headers/bloques.hpp
	$(CXX) $(CXXFLAGS) -c CargarProductos.cpp

agregar_bodega: $(OBJS_AGREGAR)
	$(CXX) $(CXXFLAGS) -o agregar_bodega $(OBJS_AGREGAR)

extraer_bodega: $(OBJS_EXTRAER)
	$(CXX) $(CXXFLAGS) -o extraer_bodega $(OBJS_EXTRAER)

verificar: $(OBJS_VERIFICAR)
	$(CXX) $(CXXFLAGS) -o verificar $(OBJS_VERIFICAR)

main.o: main.cpp Headers/almacenamiento.hpp Headers/bloques.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

almacenamiento.o: Code/almacenamiento.cpp Headers/almacenamiento.hpp Headers/bloques.hpp
	$(CXX) $(CXXFLAGS) -c Code/almacenamiento.cpp

AgregarBodega.o: AgregarBodega.cpp Headers/almacenamiento.hpp Headers/bloques.hpp
	$(CXX) $(CXXFLAGS) -c AgregarBodega.cpp

ExtraerBodega.o: ExtraerBodega.cpp Headers/almacenamiento.hpp Headers/bloques.hpp
	$(CXX) $(CXXFLAGS) -c ExtraerBodega.cpp

VerificarSistemaArchivos.o: VerificarSistemaArchivos.cpp Headers/almacenamiento.hpp Headers/bloques.hpp
	$(CXX) $(CXXFLAGS) -c VerificarSistemaArchivos.cpp

clean:
	rm -f $(OBJS) $(BIN) AgregarBodega.o ExtraerBodega.o VerificarSistemaArchivos.o CargarProductos.o agregar_bodega extraer_bodega verificar bodegas.data verificacion.data

run: $(BIN)
	./$(BIN)