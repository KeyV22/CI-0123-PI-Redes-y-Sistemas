#include "ServidorIntermedio.hpp"
#include "ServidorProductos.hpp"
#include "Buzon.hpp"
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <thread>

int main() {

    Buzon buzon;

    ServidorIntermedio intermediario(&buzon);
    ServidorProductos productos(&buzon);

    std::thread hiloProductos(&ServidorProductos::waiting, &productos);
    std::thread hiloIntermedio(&ServidorIntermedio::waiting, &intermediario);

    myMessage msg;
    bool running = true;
    std::string entrada;

    std::cout << "=== TicAmazon - Cliente de simulacion ===\n";
    std::cout << "Comandos disponibles:\n";
    std::cout << "  PEDIR:CAT\n";
    std::cout << "  PEDIR:PROD:<categoria>        (ej. PEDIR:PROD:enlatados)\n";
    std::cout << "  AGREGAR:<producto>:<cantidad> (ej. AGREGAR:Lata de refresco:2)\n";
    std::cout << "  PEDIR:FACTURA\n";
    std::cout << "  Exit\n\n";

    while (running) {
        std::cout << "> ";
        std::getline(std::cin, entrada);
        if (entrada == "Exit") {
            msg.st = CLOSE;
            running = false;
        } else {
            msg.st = CLIENTE;
        }

        msg.type = SERVIDOR_INTERMEDIO;
        strncpy(msg.message, entrada.c_str(), sizeof(msg.message) - 1);
        msg.message[sizeof(msg.message) - 1] = '\0'; // termina con caracter vacio

        buzon.Enviar(msg);

        if (msg.st == CLOSE) {
            break;
        }

        // Espera la respuesta dirigida al cliente antes de pedir el siguiente comando
        myMessage resp;
        buzon.Recibir(resp, CLIENTE);
        std::cout << "[CLIENTE] respuesta recibida: " << resp.message << std::endl;
    }

    hiloIntermedio.join();
    hiloProductos.join();
    return 0;
}