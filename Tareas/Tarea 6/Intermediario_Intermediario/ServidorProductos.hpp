#pragma once
#include <string>
#include <vector>
#include <map>

class ServidorIntermedio; //declaraciones adelantadas
struct myMessage;
class Buzon;

struct Producto{
    std::string nombre;
    int precio;
    int stock;
};

class ServidorProductos{
    public:
        // miTipo: buzon propio de esta bodega (SERVIDOR_PRODUCTOS o SERVIDOR_PRODUCTOS_B)
        // destinoRespuestas: a que buzon debe responder (SERVIDOR_INTERMEDIO o SERVIDOR_INTERMEDIO_B)
        // catalogoInicial: que categorias/productos maneja esta bodega en particular
        ServidorProductos(Buzon* b, long miTipo, long destinoRespuestas,
                           std::map<std::string, std::vector<Producto>> catalogoInicial,
                           std::string nombreLog);
        ~ServidorProductos();
        void waiting();
        void procesarSolicitud(myMessage msg);
        std::vector<std::string> categoriasDisponibles(); // usado por el intermediario para ROUTE_CAT

    private:
        bool running;
        Buzon* buzon;
        long miTipo;
        long destinoRespuestas;
        std::string nombreLog;
        std::map<std::string, std::vector<Producto>> catalogo; // categoria -> productos
        Producto* buscarProducto(const std::string& nombre);
};
