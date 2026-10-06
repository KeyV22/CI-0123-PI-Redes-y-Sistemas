# Makefile del servidor de productos - TicAmazon
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread
LDFLAGS  := -pthread

BUILDDIR := build
DATOS    := bodegas.data

BIN_SERVIDOR := servidor

FUENTES_SERVIDOR := main.cpp \
                     Servidor.cpp Manejadorconexion.cpp Router.cpp \
                     Httprequest.cpp Httpresponse.cpp Parserproforma.cpp \
                     Inventario.cpp ServicioProductos.cpp PaginasHtml.cpp \
                     Vsocket.cpp Socket.cpp \
                     almacenamiento.cpp

OBJETOS_SERVIDOR := $(patsubst %.cpp,$(BUILDDIR)/%.o,$(FUENTES_SERVIDOR))

.PHONY: all objetos clean run

all: $(BIN_SERVIDOR)

$(BIN_SERVIDOR): $(OBJETOS_SERVIDOR)
	$(CXX) $(OBJETOS_SERVIDOR) -o $@ $(LDFLAGS)

objetos: $(OBJETOS_SERVIDOR)
	@echo "El servidor compila sin errores."

$(BUILDDIR)/%.o: %.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run: $(BIN_SERVIDOR)
	./$(BIN_SERVIDOR) $(or $(PUERTO),8080) $(DATOS)

clean:
	rm -rf $(BUILDDIR) $(BIN_SERVIDOR)