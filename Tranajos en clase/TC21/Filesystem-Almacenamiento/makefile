CXX = g++
CXXFLAGS = -std=c++17 -Wall

OBJS = main.o almacenamiento.o
BIN = bodegas

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(OBJS)

main.o: main.cpp almacenamiento.hpp bloques.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

almacenamiento.o: almacenamiento.cpp almacenamiento.hpp bloques.hpp
	$(CXX) $(CXXFLAGS) -c almacenamiento.cpp

clean:
	rm -f $(OBJS) $(BIN) bodegas.data

run: $(BIN)
	./$(BIN)