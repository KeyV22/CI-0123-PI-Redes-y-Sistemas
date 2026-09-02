# Makefile simple - cliente en un solo archivo

CXX := g++
CXXFLAGS := -g -std=c++17 -c

cliente: VSocket.o Socket.o main.o
	$(CXX) -g $^ -o $@

%.o: %.cc Socket.h
	$(CXX) $(CXXFLAGS) $<

main.o: main.cpp Socket.h
	$(CXX) $(CXXFLAGS) main.cpp

clean:
	@rm -f *.o cliente
