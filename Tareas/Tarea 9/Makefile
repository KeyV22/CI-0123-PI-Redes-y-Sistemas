# Linux Makefile
# SSL server (fork + threads) + client
#

CXX := g++
CXXFLAGS := -g -Wall -std=c++11 -c
LDFLAGS := -lssl -lcrypto -pthread
DEPS := VSocket.h Socket.h SSLSocket.h
TARGETS := SSLForkMirrorServer.out SSLThreadMirrorServer.out SSLMirrorClient.out ipv4-ssl-cli.out tcp-cli.out

all: $(TARGETS)

%.o: %.cc $(DEPS)
	$(CXX) $(CXXFLAGS) $<

SSLForkMirrorServer.out: SSLForkMirrorServer.o VSocket.o Socket.o SSLSocket.o
	$(CXX) SSLForkMirrorServer.o VSocket.o Socket.o SSLSocket.o $(LDFLAGS) -o $@

SSLThreadMirrorServer.out: SSLThreadMirrorServer.o VSocket.o Socket.o SSLSocket.o
	$(CXX) SSLThreadMirrorServer.o VSocket.o Socket.o SSLSocket.o $(LDFLAGS) -o $@

SSLMirrorClient.out: SSLMirrorClient.o VSocket.o Socket.o SSLSocket.o
	$(CXX) SSLMirrorClient.o VSocket.o Socket.o SSLSocket.o $(LDFLAGS) -o $@

ipv4-ssl-cli.out: ipv4-ssl-cli.o VSocket.o Socket.o SSLSocket.o
	$(CXX) ipv4-ssl-cli.o VSocket.o Socket.o SSLSocket.o $(LDFLAGS) -o $@

tcp-cli.out: tcp-cli.o VSocket.o Socket.o SSLSocket.o
	$(CXX) tcp-cli.o VSocket.o Socket.o SSLSocket.o $(LDFLAGS) -o $@

clean:
	rm -f *.o *.out
