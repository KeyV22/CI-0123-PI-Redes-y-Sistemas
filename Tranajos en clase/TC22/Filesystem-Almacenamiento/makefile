CXX = g++
CXXFLAGS = -std=c++17 -Wall

OBJS = main.o almacenamiento.o
BIN = bodegas
LISTAR = listar

all: $(BIN) $(LISTAR)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(OBJS)

$(LISTAR): listar.o almacenamiento.o
	$(CXX) $(CXXFLAGS) -o $(LISTAR) listar.o almacenamiento.o

main.o: main.cpp almacenamiento.hpp bloques.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

listar.o: listar.cpp almacenamiento.hpp bloques.hpp
	$(CXX) $(CXXFLAGS) -c listar.cpp

almacenamiento.o: almacenamiento.cpp almacenamiento.hpp bloques.hpp
	$(CXX) $(CXXFLAGS) -c almacenamiento.cpp

clean:
	rm -f $(OBJS) listar.o $(BIN) $(LISTAR) bodegas.data

run: $(BIN)
	./$(BIN)