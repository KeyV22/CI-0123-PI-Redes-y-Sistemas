CXX = g++
CXXFLAGS = -std=c++17 -Wall

OBJS = main.o almacenamiento.o
BIN = bodegas

OBJS_AGREGAR = AgregarBodega.o almacenamiento.o
OBJS_EXTRAER = ExtraerBodega.o almacenamiento.o

all: $(BIN) agregar_bodega extraer_bodega

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(OBJS)

agregar_bodega: $(OBJS_AGREGAR)
	$(CXX) $(CXXFLAGS) -o agregar_bodega $(OBJS_AGREGAR)

extraer_bodega: $(OBJS_EXTRAER)
	$(CXX) $(CXXFLAGS) -o extraer_bodega $(OBJS_EXTRAER)

main.o: main.cpp Headers/almacenamiento.hpp Headers/bloques.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

almacenamiento.o: Code/almacenamiento.cpp Headers/almacenamiento.hpp Headers/bloques.hpp
	$(CXX) $(CXXFLAGS) -c Code/almacenamiento.cpp

AgregarBodega.o: AgregarBodega.cpp Headers/almacenamiento.hpp Headers/bloques.hpp
	$(CXX) $(CXXFLAGS) -c AgregarBodega.cpp

ExtraerBodega.o: ExtraerBodega.cpp Headers/almacenamiento.hpp Headers/bloques.hpp
	$(CXX) $(CXXFLAGS) -c ExtraerBodega.cpp

clean:
	rm -f $(OBJS) $(BIN) AgregarBodega.o ExtraerBodega.o agregar_bodega extraer_bodega bodegas.data

run: $(BIN)
	./$(BIN)