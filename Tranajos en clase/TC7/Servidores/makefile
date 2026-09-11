# Linux Makefile
#

CC  := gcc
CXX := g++
CFLAGS := -g -c
CXXFLAGS := -g -c -std=c++11
LDFLAGS := -pthread
DEPS := Socket.hpp VSocket.hpp
TARGETS := ForkMirrorServer.out MirrorClient.out ThreadMirrorServer.out

all: $(TARGETS)

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $<

ForkMirrorServer.out:	ForkMirrorServer.o VSocket.o Socket.o
	$(CXX) ForkMirrorServer.o VSocket.o Socket.o $(LDFLAGS) -o $@

MirrorClient.out:	MirrorClient.o VSocket.o Socket.o
	$(CXX) MirrorClient.o VSocket.o Socket.o $(LDFLAGS) -o $@

ThreadMirrorServer.out:	ThreadMirrorServer.o VSocket.o Socket.o
	$(CXX) ThreadMirrorServer.o VSocket.o Socket.o $(LDFLAGS) -o $@

clean:
	rm -f *.o *.out
