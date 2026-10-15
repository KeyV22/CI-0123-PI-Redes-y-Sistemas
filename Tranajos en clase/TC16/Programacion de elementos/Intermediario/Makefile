# Makefile del intermediario - TicAmazon
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread
LDFLAGS  := -pthread

BUILDDIR := build
BIN      := intermediario

FUENTES := main.cpp \
           Servidor.cpp Manejadorconexion.cpp Router.cpp \
           Httprequest.cpp Httpresponse.cpp Parserproforma.cpp \
           ServicioProductos.cpp ServicioProductosRemoto.cpp PaginasHtml.cpp \
           Vsocket.cpp Socket.cpp

OBJETOS := $(patsubst %.cpp,$(BUILDDIR)/%.o,$(FUENTES))

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJETOS)
	$(CXX) $(OBJETOS) -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: %.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run: $(BIN)
	./$(BIN) $(or $(PUERTO),8080) $(or $(HOST_PRODUCTOS),127.0.0.1) $(or $(PUERTO_PRODUCTOS),9090)

clean:
	rm -rf $(BUILDDIR) $(BIN)