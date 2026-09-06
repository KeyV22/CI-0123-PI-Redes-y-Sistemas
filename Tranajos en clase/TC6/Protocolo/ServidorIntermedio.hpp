#pragma once
#include <string>
#include <vector>
#include <map>

class Cliente;
class ServidorProductos;
struct Message;
class Buzon;

struct ItemCarrito{
    std::string nombre;
    int cantidad;
    int precio;
};

class ServidorIntermedio {
    public:
        ServidorIntermedio(Buzon* b);
        void connect(ServidorProductos* servp);
        ~ServidorIntermedio();
        void waiting();
    private:
        Buzon* buzon;
        ServidorProductos* serv;

        std::vector<ItemCarrito> carrito;
        std::map<std::string, int> precioCache;   // recordado de las respuestas PROD:
        std::string ultimoComandoCliente;          // "AGREGAR" | "FACTURA" | "PASSTHROUGH"
        std::string nombrePendiente;
        int cantidadPendiente = 0;
        int siguienteIdOrden = 1;

        void agarrarPrecios(const std::string& listaProd);
};