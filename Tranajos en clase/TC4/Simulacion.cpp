#include "ServidorIntermedio.hpp"
#include "ServidorProductos.hpp"
#include "Buzon.hpp"
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>

int main() {

    Buzon buzon;

    ServidorIntermedio intermediario(&buzon);
    ServidorProductos productos(&buzon);

    pid_t pid1 = fork(); //crear proceso

    if (pid1 == 0) { //encargado de ejecutar el servidor de productod
        productos.waiting();
        return 0;
    }

    pid_t pid2 = fork();

    if (pid2 == 0) {//encargado del intermediario
        intermediario.waiting();
        return 0;
    }

    myMessage msg;
    bool running = true;
    std::string op;

    std::cout << "=== TicAmazon - Cliente de simulacion ===\n";
    std::cout << "Use: see_categories\n";
    std::cout << "Use: GET nombre_categoria\n";
    std::cout << "Use: Exit\n";
    while (running) {
        std::getline(std::cin, entrada);
        if (entrada == "Exit") {
            msg.st = CLOSE;
            running = false;
        } else {
            msg.st = CLIENTE;
        }

        msg.type = SERVIDOR_INTERMEDIO;
        strncpy(msg.message, entrada.c_str(), sizeof(msg.message) - 1);
        msg.message[sizeof(msg.message) - 1] = '\0'; //termina con caracter vacio

        buzon.Enviar(msg);

        if (msg.st == CLOSE) {
            break;
        }
    }
    return 0;
}