#include "ServidorIntermedio.hpp"
#include "ServidorProductos.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include "Buzon.hpp"

ServidorIntermedio::ServidorIntermedio(Buzon* b, long miTipo_, long tipoMiBodega_, long tipoPeer_,
                                        std::string miId_, std::string idPeer_, std::string nombreLog_) {
    buzon = b;
    miTipo = miTipo_;
    tipoMiBodega = tipoMiBodega_;
    tipoPeer = tipoPeer_;
    miId = miId_;
    idPeer = idPeer_;
    nombreLog = nombreLog_;
}

ServidorIntermedio::~ServidorIntermedio(){
}

void ServidorIntermedio::connect(ServidorProductos* servp) {
    this->serv = servp;
}

void ServidorIntermedio::cachearPrecios(const std::string& listaProd) {
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

// Envia un mensaje a mi propia bodega y espera su respuesta.
// Mismo patron sincrono "REQUEST -> Enviar / RESPONSE -> Recibir"; aqui lo saco a una funcion aparte
// porque ahora lo necesitamos en mas de un lugar (cliente normal Y ROUTE_CAT/FWD_PROD).
std::string ServidorIntermedio::preguntarleAMiBodega(const std::string& mensaje) {
    myMessage msg;
    msg.type = tipoMiBodega;
    msg.st = REQUEST;
    strncpy(msg.message, mensaje.c_str(), sizeof(msg.message) - 1);
    msg.message[sizeof(msg.message) - 1] = '\0';
    std::cout << "[" << nombreLog << "] -> bodega local: " << mensaje << std::endl;
    buzon->Enviar(msg);

    myMessage resp;
    buzon->Recibir(resp, miTipo); // mi bodega siempre responde a mi propio buzon
    std::string respuesta(resp.message);
    std::cout << "[" << nombreLog << "] <- bodega local: " << respuesta << std::endl;
    return respuesta;
}

// Igual que la anterior, pero hablando con el intermediario PAR en vez de con
// mi bodega.
// ROUTE_CAT/FWD_PROD/PING_PEER salen por aqui.
std::string ServidorIntermedio::preguntarleAMiPeer(const std::string& mensaje) {
    myMessage msg;
    msg.type = tipoPeer;
    msg.st = REQUEST;
    strncpy(msg.message, mensaje.c_str(), sizeof(msg.message) - 1);
    msg.message[sizeof(msg.message) - 1] = '\0';
    std::cout << "[" << nombreLog << "] -> peer (" << idPeer << "): " << mensaje << std::endl;
    buzon->Enviar(msg);

    myMessage resp;
    buzon->Recibir(resp, miTipo); // el peer responde a mi propio buzon
    std::string respuesta(resp.message);
    std::cout << "[" << nombreLog << "] <- peer (" << idPeer << "): " << respuesta << std::endl;
    return respuesta;
}

void ServidorIntermedio::waiting() {
    myMessage msg;
    while(true) {
        buzon->Recibir(msg, miTipo);

        if (msg.st == CLIENTE) {
            atenderCliente(msg);

        } else if (msg.st == REQUEST) {
            // Esto NO viene de un cliente ni es la respuesta de mi bodega:
            // es una solicitud de mi intermediario PAR (ROUTE_CAT / FWD_PROD / PING_PEER)
            atenderPeer(msg);

        } else if (msg.st == CLOSE) {
            std::cout << "[" << nombreLog << "] cerrando sistema" << std::endl;
            myMessage cerrarBodega;
            cerrarBodega.type = tipoMiBodega;
            cerrarBodega.st = CLOSE;
            buzon->Enviar(cerrarBodega);
            break;
        }
        // msg.st == RESPONSE nunca deberia llegar aqui: las respuestas de la
        // bodega y del peer se consumen directamente dentro de
        // preguntarleAMiBodega()/preguntarleAMiPeer(), no en este bucle.
    }
}

// ----------------------------------------------------------------------------
// Atiende una solicitud de un CLIENTE real (PEDIR:CAT, PEDIR:PROD, AGREGAR, PEDIR:FACTURA)
// ----------------------------------------------------------------------------
void ServidorIntermedio::atenderCliente(myMessage msg) {
    std::string contenido(msg.message);
    std::cout << std::endl << "[" << nombreLog << "] solicitud del cliente: " << contenido << std::endl;

    std::string respuestaCliente;

    if (contenido == "PEDIR:CAT") {
        respuestaCliente = preguntarleAMiBodega(contenido);

    } else if (contenido.rfind("PEDIR:PROD:", 0) == 0) {
        std::string categoria = contenido.substr(11);
        std::string respuestaBodega = preguntarleAMiBodega(contenido);

        if (respuestaBodega.rfind("PROD:", 0) == 0) {
            // Mi propia bodega SI tenia la categoria: no hace falta preguntarle a nadie mas
            cachearPrecios(respuestaBodega.substr(5));
            respuestaCliente = respuestaBodega;

        } else {
            // Mi bodega no la tiene -> protocolo Intermediario-Intermediario:
            // le pregunto a mi peer si el si la tiene (ROUTE_CAT), y si dice
            // que si, le pido que me traiga el listado (FWD_PROD).
            std::string routeAck = preguntarleAMiPeer("ROUTE_CAT:" + categoria);
            // formato esperado: ROUTE_ACK:<categoria>:SI  o  ROUTE_ACK:<categoria>:NO
            bool tienePeer = routeAck.size() >= 2 && routeAck.substr(routeAck.size() - 2) == "SI";

            if (tienePeer) {
                std::string fwd = preguntarleAMiPeer("FWD_PROD:" + categoria);
                if (fwd.rfind("FWD_PROD_LIST:", 0) == 0) {
                    std::string lista = fwd.substr(14);
                    cachearPrecios(lista);
                    respuestaCliente = "PROD:" + lista; // se traduce de vuelta al formato que el cliente conoce
                } else {
                    respuestaCliente = "ERROR:404";
                }
            } else {
                // Con un solo peer configurado, si dice NO ya no queda a quien mas preguntarle.
                // (Con mas de un peer, aqui seguiria con el siguiente de la lista, en orden fijo.)
                respuestaCliente = "ERROR:404";
            }
        }

    } else if (contenido.rfind("AGREGAR:", 0) == 0) {
        std::string resto = contenido.substr(8);
        size_t pos = resto.rfind(':');
        nombrePendiente = (pos == std::string::npos) ? resto : resto.substr(0, pos);
        cantidadPendiente = (pos == std::string::npos) ? 0 : std::stoi(resto.substr(pos + 1));

        std::string respuestaBodega = preguntarleAMiBodega("RESERVAR:" + nombrePendiente + ":" + std::to_string(cantidadPendiente));

        if (respuestaBodega == "ERROR:404") {
            // El producto no vive en mi bodega local -> se lo pido reservar a mi peer
            // (protocolo Intermediario-Intermediario: FWD_RESERVE / FWD_RESERVE_OK / FWD_RESERVE_FAIL)
            std::string respuestaPeer = preguntarleAMiPeer("FWD_RESERVE:" + nombrePendiente + ":" + std::to_string(cantidadPendiente));
            if (respuestaPeer == "FWD_RESERVE_OK") {
                int precio = precioCache.count(nombrePendiente) ? precioCache[nombrePendiente] : 0;
                carrito.push_back({nombrePendiente, cantidadPendiente, precio});
                respuestaCliente = "OK";
            } else if (respuestaPeer.rfind("FWD_RESERVE_FAIL:", 0) == 0) {
                respuestaCliente = "ERROR:" + respuestaPeer.substr(17);
            } else {
                respuestaCliente = "ERROR:404";
            }
        } else {
            if (respuestaBodega == "OK") {
                int precio = precioCache.count(nombrePendiente) ? precioCache[nombrePendiente] : 0;
                carrito.push_back({nombrePendiente, cantidadPendiente, precio});
            }
            respuestaCliente = respuestaBodega;
        }

    } else if (contenido == "PEDIR:FACTURA") {
        preguntarleAMiBodega("CONFIRMAR:" + std::to_string(siguienteIdOrden));
        siguienteIdOrden++;

        std::string detalle;
        long total = 0;
        for (auto& item : carrito) {
            if (!detalle.empty()) detalle += ";";
            detalle += item.nombre + "x" + std::to_string(item.cantidad);
            total += (long)item.cantidad * item.precio;
        }
        respuestaCliente = "FACTURA:" + detalle + ":" + std::to_string(total);

    } else {
        respuestaCliente = "ERROR:400";
    }

    myMessage resp;
    resp.type = CLIENTE;
    strncpy(resp.message, respuestaCliente.c_str(), sizeof(resp.message) - 1);
    resp.message[sizeof(resp.message) - 1] = '\0';
    std::cout << "[" << nombreLog << "] enviando al Cliente: " << respuestaCliente << std::endl << std::endl;
    buzon->Enviar(resp);
}

// ----------------------------------------------------------------------------
// Atiende una solicitud de mi intermediario PAR (protocolo Intermediario-Intermediario)
// ----------------------------------------------------------------------------
void ServidorIntermedio::atenderPeer(myMessage msg) {
    std::string contenido(msg.message);
    std::cout << "[" << nombreLog << "] solicitud de peer (" << idPeer << "): " << contenido << std::endl;

    std::string respuesta;

    if (contenido.rfind("ROUTE_CAT:", 0) == 0) {
        std::string categoria = contenido.substr(10);
        std::string catLocales = preguntarleAMiBodega("PEDIR:CAT"); // "CAT:cat1,cat2,..."
        bool laTengo = false;
        if (catLocales.rfind("CAT:", 0) == 0) {
            std::stringstream ss(catLocales.substr(4));
            std::string nombreCat;
            while (std::getline(ss, nombreCat, ',')) {
                if (nombreCat == categoria) { laTengo = true; break; }
            }
        }
        respuesta = "ROUTE_ACK:" + categoria + ":" + (laTengo ? "SI" : "NO");

    } else if (contenido.rfind("FWD_PROD:", 0) == 0) {
        std::string categoria = contenido.substr(9);
        std::string respuestaBodega = preguntarleAMiBodega("PEDIR:PROD:" + categoria);
        if (respuestaBodega.rfind("PROD:", 0) == 0) {
            respuesta = "FWD_PROD_LIST:" + respuestaBodega.substr(5);
        } else {
            respuesta = "FWD_PROD_LIST_EMPTY";
        }

    } else if (contenido.rfind("FWD_RESERVE:", 0) == 0) {
        std::string resto = contenido.substr(12);
        std::string respuestaBodega = preguntarleAMiBodega("RESERVAR:" + resto);
        if (respuestaBodega == "OK") {
            respuesta = "FWD_RESERVE_OK";
        } else {
            // ERROR:404 o ERROR:STOCK_INSUFICIENTE -> se le avisa al que pregunto cual fue el motivo
            std::string motivo = (respuestaBodega.rfind("ERROR:", 0) == 0) ? respuestaBodega.substr(6) : "400";
            respuesta = "FWD_RESERVE_FAIL:" + motivo;
        }

    } else if (contenido.rfind("PING_PEER:", 0) == 0) {
        respuesta = "PONG_PEER:" + miId;

    } else {
        respuesta = "ERROR:400";
    }

    myMessage resp;
    resp.type = tipoPeer; // le respondo al buzon de quien me pregunto
    strncpy(resp.message, respuesta.c_str(), sizeof(resp.message) - 1);
    resp.message[sizeof(resp.message) - 1] = '\0';
    std::cout << "[" << nombreLog << "] respondiendo a peer: " << respuesta << std::endl;
    buzon->Enviar(resp);
}
