#pragma once
#include <string>
#include <vector>
#include <map>

class ServidorIntermedio;//declaraciones adelantadas
struct myMessage;
class Buzon;

struct Producto{
    std::string nombre;
    int precio;
    int stock;
};

class ServidorProductos{
    public:
        ServidorProductos(Buzon* b);
        ~ServidorProductos();
        void waiting();
        void procesarSolicitud(myMessage msg);
    
    private:
        bool running;
        Buzon* buzon;
        std::map<std::string, std::vector<Producto>> catalogo; // categoria -> productos
        void inicializarCatalogo();
        Producto* buscarProducto(const std::string& nombre);
};