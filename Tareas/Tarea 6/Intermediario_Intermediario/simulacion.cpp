#include "ServidorIntermedio.hpp"
#include "ServidorProductos.hpp"
#include "Buzon.hpp"
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <map>
#include <vector>

int main() {

    Buzon buzon;

    // ---- Catalogo de la bodega A ----
    std::map<std::string, std::vector<Producto>> catalogoA;
    catalogoA["enlatados"] = { {"Lata de refresco", 40, 15}, {"Atun Suli", 25, 8}, {"Sardina vencida", 30, 20} };
    catalogoA["bebidas"]   = { {"Powercito", 120, 10}, {"Jetcito", 95, 5}, {"Monstercito", 15, 50} };

    // ---- Catalogo de la bodega B (categorias que A NO tiene) ----
    std::map<std::string, std::vector<Producto>> catalogoB;
    catalogoB["pastas"]  = { {"Pasta de palito", 60, 12}, {"Pasta de caracolito", 20, 30}, {"Pasta gruesa", 10, 40} };
    catalogoB["postres"] = { {"Flan de coco", 45, 6} };

    ServidorProductos bodegaA(&buzon, SERVIDOR_PRODUCTOS,   SERVIDOR_INTERMEDIO,   catalogoA, "BodegaA");
    ServidorProductos bodegaB(&buzon, SERVIDOR_PRODUCTOS_B, SERVIDOR_INTERMEDIO_B, catalogoB, "BodegaB");

    ServidorIntermedio intermediarioA(&buzon, SERVIDOR_INTERMEDIO,   SERVIDOR_PRODUCTOS,   SERVIDOR_INTERMEDIO_B,
                                       "IntermediarioA", "IntermediarioB", "IntermediarioA");
    ServidorIntermedio intermediarioB(&buzon, SERVIDOR_INTERMEDIO_B, SERVIDOR_PRODUCTOS_B, SERVIDOR_INTERMEDIO,
                                       "IntermediarioB", "IntermediarioA", "IntermediarioB");

    intermediarioA.connect(&bodegaA);
    intermediarioB.connect(&bodegaB);

    std::thread hiloBodegaA(&ServidorProductos::waiting, &bodegaA);
    std::thread hiloBodegaB(&ServidorProductos::waiting, &bodegaB);
    std::thread hiloIntermediarioA(&ServidorIntermedio::waiting, &intermediarioA);
    std::thread hiloIntermediarioB(&ServidorIntermedio::waiting, &intermediarioB);

    myMessage msg;
    bool running = true;
    std::string entrada;

    std::cout << "=== TicAmazon - Cliente (habla solo con IntermediarioA) ===\n";
    std::cout << "IntermediarioA tiene: enlatados, bebidas\n";
    std::cout << "IntermediarioB tiene: pastas, postres  (A le pregunta a B si no la tiene)\n\n";
    std::cout << "Comandos: PEDIR:CAT | PEDIR:PROD:<categoria> | AGREGAR:<producto>:<cantidad> | PEDIR:FACTURA | Exit\n\n";

    while (running) {
        std::cout << "> ";
        std::getline(std::cin, entrada);
        if (entrada == "Exit") {
            msg.st = CLOSE;
            running = false;
        } else {
            msg.st = CLIENTE;
        }

        msg.type = SERVIDOR_INTERMEDIO; // el cliente SOLO le habla a IntermediarioA
        strncpy(msg.message, entrada.c_str(), sizeof(msg.message) - 1);
        msg.message[sizeof(msg.message) - 1] = '\0';
        buzon.Enviar(msg);

        if (msg.st == CLOSE) break;

        myMessage resp;
        buzon.Recibir(resp, CLIENTE);
        std::cout << "[CLIENTE] respuesta recibida: " << resp.message << std::endl;
    }

    // IntermediarioA ya recibio CLOSE y va a cerrar su propia bodega (BodegaA).
    // Como IntermediarioB y BodegaB no tienen cliente propio, hay que avisarles aparte.
    myMessage cerrarB;
    cerrarB.type = SERVIDOR_INTERMEDIO_B;
    cerrarB.st = CLOSE;
    buzon.Enviar(cerrarB);

    hiloIntermediarioA.join();
    hiloIntermediarioB.join();
    hiloBodegaA.join();
    hiloBodegaB.join();
    return 0;
}
