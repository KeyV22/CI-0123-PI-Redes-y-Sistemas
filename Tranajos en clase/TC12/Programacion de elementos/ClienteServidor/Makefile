# Makefile del cliente - TicAmazon
#
# Compila el programa cliente que se conecta al servidor de productos:
# lista categorias, permite armar un carrito y al final envia la
# proforma por POST.

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
LDFLAGS  :=

BUILDDIR := build

BIN_CLIENTE := cliente

FUENTES_CLIENTE := main.cpp \
                    Cliente.cpp Producto.cpp Carrito.cpp \
                    Vsocket.cpp Socket.cpp

OBJETOS_CLIENTE := $(patsubst %.cpp,$(BUILDDIR)/%.o,$(FUENTES_CLIENTE))

.PHONY: all objetos clean run

all: $(BIN_CLIENTE)

$(BIN_CLIENTE): $(OBJETOS_CLIENTE)
	$(CXX) $(OBJETOS_CLIENTE) -o $@ $(LDFLAGS)

# make objetos: compila sin enlazar (chequeo rapido mientras se desarrolla)
objetos: $(OBJETOS_CLIENTE)
	@echo "El cliente compila sin errores."

$(BUILDDIR)/%.o: %.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# make run IP=10.1.2.3 PUERTO=8080
run: $(BIN_CLIENTE)
	./$(BIN_CLIENTE) $(IP) $(or $(PUERTO),8080)

clean:
	rm -rf $(BUILDDIR) $(BIN_CLIENTE)