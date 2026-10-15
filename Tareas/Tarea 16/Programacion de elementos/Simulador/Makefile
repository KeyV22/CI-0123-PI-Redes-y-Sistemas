CXX = g++
CXXFLAGS = -std=c++11 -g -Wall -Irecursos -MMD -MP
LDFLAGS = -pthread
FUENTES = $(wildcard recursos/*.cc) Principal.cc
OBJETOS = $(patsubst %.cc,objetos/%.o,$(FUENTES))

all: Simulador.out

Simulador.out: $(OBJETOS)
	@mkdir -p bitacora bodegas
	$(CXX) $^ $(LDFLAGS) -o $@

objetos/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

limpiar:
	rm -rf objetos Simulador.out

-include $(OBJETOS:.o=.d)
