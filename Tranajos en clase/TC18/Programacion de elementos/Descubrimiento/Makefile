CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread
COMUN    := Vsocket.cpp Socket.cpp

all: buscar responder

buscar: BuscarPorBroadcast.cpp $(COMUN)
	$(CXX) $(CXXFLAGS) $^ -o $@

responder: ResponderPorBroadcast.cpp $(COMUN)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f buscar responder

.PHONY: all clean