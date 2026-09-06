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

// Extrae "nombre,precio" de una respuesta PROD:<lista> y los guarda en
// precioCache, para poder calcular el total de la factura mas adelante
// sin tener que volver a preguntarle el precio a la bodega.
void ServidorIntermedio::agarrarPrecios(const std::string& listaProd) {
    std::stringstream ss(listaProd);
    std::string item;
    while (std::getline(ss, item, ';')) {
        size_t pos = item.rfind(',');
        if (pos == std::string::npos) continue;
        std::string nombre = item.substr(0, pos);
        int precio = std::stoi(item.substr(pos + 1));
        precioCache[nombre] = precio;
    }
}

void ServidorIntermedio::waiting() {
    myMessage msg;
    while(true) {
        buzon->Recibir(msg, SERVIDOR_INTERMEDIO);

        if (msg.st == CLIENTE) {
            std::string contenido(msg.message);
            std::cout << std::endl << "[INTERMEDIARIO] solicitud del cliente: " << contenido << std::endl;

            std::string comandoParaBodega = contenido; // por defecto se reenvia igual (PEDIR:CAT / PEDIR:PROD:x)

            if (contenido.rfind("AGREGAR:", 0) == 0) {
                // AGREGAR:<producto>:<cantidad> -> se traduce a RESERVAR:<producto>:<cantidad>
                std::string resto = contenido.substr(8);
                size_t pos = resto.rfind(':');
                nombrePendiente = (pos == std::string::npos) ? resto : resto.substr(0, pos);
                cantidadPendiente = (pos == std::string::npos) ? 0 : std::stoi(resto.substr(pos + 1));
                comandoParaBodega = "RESERVAR:" + nombrePendiente + ":" + std::to_string(cantidadPendiente);
                ultimoComandoCliente = "AGREGAR";

            } else if (contenido == "PEDIR:FACTURA") {
                comandoParaBodega = "CONFIRMAR:" + std::to_string(siguienteIdOrden);
                ultimoComandoCliente = "FACTURA";

            } else {
                ultimoComandoCliente = "PASSTHROUGH"; // PEDIR:CAT o PEDIR:PROD:<categoria>
            }

            strncpy(msg.message, comandoParaBodega.c_str(), sizeof(msg.message) - 1);
            msg.message[sizeof(msg.message) - 1] = '\0';
            msg.type = SERVIDOR_PRODUCTOS;
            msg.st = REQUEST;
            std::cout << "[INTERMEDIARIO] reenviando a Bodega: " << msg.message << std::endl;
            buzon->Enviar(msg);

        } else if (msg.st == RESPONSE) {
            std::string respuestaBodega(msg.message);
            std::cout << "[INTERMEDIARIO] respuesta de Bodega: " << respuestaBodega << std::endl;

            std::string respuestaCliente = respuestaBodega;

            if (respuestaBodega.rfind("PROD:", 0) == 0) {
                cachearPrecios(respuestaBodega.substr(5));

            } else if (ultimoComandoCliente == "AGREGAR") {
                if (respuestaBodega == "OK") {
                    int precio = precioCache.count(nombrePendiente) ? precioCache[nombrePendiente] : 0;
                    carrito.push_back({nombrePendiente, cantidadPendiente, precio});
                }
                respuestaCliente = respuestaBodega; // OK, ERROR:STOCK_INSUFICIENTE o ERROR:404

            } else if (ultimoComandoCliente == "FACTURA") {
                std::string detalle;
                long total = 0;
                for (auto& item : carrito) {
                    if (!detalle.empty()) detalle += ";";
                    detalle += item.nombre + "x" + std::to_string(item.cantidad);
                    total += (long)item.cantidad * item.precio;
                }
                respuestaCliente = "FACTURA:" + detalle + ":" + std::to_string(total);
                siguienteIdOrden++;
            }

            strncpy(msg.message, respuestaCliente.c_str(), sizeof(msg.message) - 1);
            msg.message[sizeof(msg.message) - 1] = '\0';
            std::cout << "[INTERMEDIARIO] enviando al Cliente: " << respuestaCliente << std::endl << std::endl;
            msg.type = CLIENTE;
            buzon->Enviar(msg);

        } else if (msg.st == CLOSE) {
            std::cout << "[INTERMEDIARIO] cerrando sistema" << std::endl;
            msg.type = SERVIDOR_PRODUCTOS;
            buzon->Enviar(msg);
            break;
        }
    }
}