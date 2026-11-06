# Makefile del servidor de productos - TicAmazon
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread
LDFLAGS  := -pthread

BUILDDIR := build
PUERTO_PROD := 9090
DATOS    := bodegas.data
NOMBRE      := PC1

BIN_SERVIDOR := servidor

FUENTES_SERVIDOR := main.cpp \
                     Servidor.cpp Manejadorconexion.cpp Router.cpp \
                     Httprequest.cpp Httpresponse.cpp Parserproforma.cpp \
                     Inventario.cpp ServicioProductos.cpp ServicioProductosLocal.cpp PaginasHtml.cpp \
                     Vsocket.cpp Socket.cpp SSLSocket.cpp\
                     almacenamiento.cpp ClienteRegistro.cpp Descubrimiento.cpp

OBJETOS_SERVIDOR := $(patsubst %.cpp,$(BUILDDIR)/%.o,$(FUENTES_SERVIDOR))

.PHONY: all objetos clean run

all: $(BIN_SERVIDOR)

$(BIN_SERVIDOR): $(OBJETOS_SERVIDOR)
	$(CXX) $(OBJETOS_SERVIDOR) -o $@ $(LDFLAGS) -lssl -lcrypto

objetos: $(OBJETOS_SERVIDOR)
	@echo "El servidor compila sin errores."

$(BUILDDIR)/%.o: %.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run: $(BIN_SERVIDOR)
	./$(BIN_SERVIDOR) $(PUERTO_PROD) $(DATOS) $(NOMBRE)

clean:
	rm -rf $(BUILDDIR) $(BIN_SERVIDOR)