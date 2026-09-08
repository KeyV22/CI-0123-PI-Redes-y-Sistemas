#include "ServidorProductos.hpp"
#include "ServidorIntermedio.hpp"
#include <cstring>
#include <iostream>
#include "Buzon.hpp"

ServidorProductos::ServidorProductos(Buzon* b, long miTipo_, long destinoRespuestas_,
                                      std::map<std::string, std::vector<Producto>> catalogoInicial,
                                      std::string nombreLog_){
    running = true;
    buzon = b;
    miTipo = miTipo_;
    destinoRespuestas = destinoRespuestas_;
    nombreLog = nombreLog_;
    catalogo = catalogoInicial;
}

ServidorProductos::~ServidorProductos(){
}

Producto* ServidorProductos::buscarProducto(const std::string& nombre){
    for (auto& categoria : catalogo) {
        for (auto& p : categoria.second) {
            if (p.nombre == nombre) return &p;
        }
    }
    return nullptr;
}

std::vector<std::string> ServidorProductos::categoriasDisponibles(){
    std::vector<std::string> resultado;
    for (auto& categoria : catalogo) resultado.push_back(categoria.first);
    return resultado;
}

void ServidorProductos::waiting(){
    myMessage msg;
    while(running){
        buzon->Recibir(msg, miTipo);
        if(msg.st==REQUEST){
            std::cout << "[" << nombreLog << "] solicitud recibida: " << msg.message << std::endl;
            procesarSolicitud(msg);
        }else if(msg.st==CLOSE){
            std::cout << "[" << nombreLog << "] cerrando servidor" << std::endl;
            running=false;
        }
    }
}

// Interpreta mensajes del protocolo:
//   PEDIR:CAT                      -> CAT:<lista de categorias>
//   PEDIR:PROD:<categoria>         -> PROD:<nombre,precio;...>  o  ERROR:404
//   RESERVAR:<producto>:<cantidad> -> OK | ERROR:404 | ERROR:STOCK_INSUFICIENTE
//   CONFIRMAR:<id_orden>           -> OK
void ServidorProductos::procesarSolicitud(myMessage msg){
    std::string cadena = msg.message;
    std::string respuesta;

    if (cadena == "PEDIR:CAT") {
        std::string lista;
        for (auto& categoria : catalogo) {
            if (!lista.empty()) lista += ",";
            lista += categoria.first;
        }
        respuesta = "CAT:" + lista;

    } else if (cadena.rfind("PEDIR:PROD:", 0) == 0) {
        std::string categoria = cadena.substr(11);
        if (catalogo.count(categoria) == 0) {
            respuesta = "ERROR:404";
        } else {
            std::string lista;
            for (auto& p : catalogo[categoria]) {
                if (!lista.empty()) lista += ";";
                lista += p.nombre + "," + std::to_string(p.precio);
            }
            respuesta = "PROD:" + lista;
        }

    } else if (cadena.rfind("RESERVAR:", 0) == 0) {
        std::string resto = cadena.substr(9);
        size_t pos = resto.rfind(':');
        std::string nombreProducto = (pos == std::string::npos) ? resto : resto.substr(0, pos);
        int cantidad = (pos == std::string::npos) ? 0 : std::stoi(resto.substr(pos + 1));

        Producto* p = buscarProducto(nombreProducto);
        if (!p) {
            respuesta = "ERROR:404";
        } else if (p->stock < cantidad) {
            respuesta = "ERROR:STOCK_INSUFICIENTE";
        } else {
            p->stock -= cantidad;
            respuesta = "OK";
        }

    } else if (cadena.rfind("CONFIRMAR:", 0) == 0) {
        respuesta = "OK";

    } else {
        respuesta = "ERROR:400";
    }

    myMessage resp;
    resp.type = destinoRespuestas;
    resp.st = RESPONSE;
    strncpy(resp.message, respuesta.c_str(), sizeof(resp.message) - 1);
    resp.message[sizeof(resp.message) - 1] = '\0';
    buzon->Enviar(resp);
    std::cout << "[" << nombreLog << "] enviando respuesta: " << respuesta << std::endl;
}
