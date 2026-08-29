#include "ServidorIntermedio.hpp"
#include "ServidorProductos.hpp"
#include <iostream>
#include <cstring>
#include "Buzon.hpp"

ServidorIntermedio::ServidorIntermedio(Buzon* b) {
    this->buzon = b;
}

ServidorIntermedio::~ServidorIntermedio(){
    
}
void ServidorIntermedio::connect(ServidorProductos* servp) {
    this->serv = servp;
}


void ServidorIntermedio::waiting() {
    myMessage msg;
    while(true) {
        buzon->Recibir(msg, SERVIDOR_INTERMEDIO);
        if (msg.st == CLIENTE) {
            std::cout << std::endl;
            std::cout << "[INTERMEDIARIO] Solicitud del cliente recibida! Enviando solicitud al servidor\n";
            msg.type = SERVIDOR_PRODUCTOS;
            msg.st = REQUEST;
            buzon->Enviar(msg);
        } else if(msg.st == RESPONSE) {
            std::cout << "[INTERMEDIARIO] Respuesta del servidor enviada al cliente\n" << std::endl;
            std::cout << "[Cliente] Respuesta del servidor:\n" << msg.message << std::endl;
            msg.type = CLIENTE;
            buzon->Enviar(msg);
        } else if(msg.st == CLOSE) {
            std::cout << "[INTERMEDIARIO] Cerrando sistema\n";
            msg.type = SERVIDOR_PRODUCTOS;
            buzon->Enviar(msg);
            break;
        }
    }
}