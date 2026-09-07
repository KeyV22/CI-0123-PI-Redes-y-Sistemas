#include "ServidorProductos.hpp"
#include "ServidorIntermedio.hpp"
#include <cstring>
#include <iostream>
#include "Buzon.hpp"

ServidorProductos::ServidorProductos(Buzon* b){
    running=true;
    buzon=b;
    inicializarCatalogo();
}

ServidorProductos::~ServidorProductos(){

}

void ServidorProductos::inicializarCatalogo(){
    catalogo["enlatados"] = {
        {"Lata de refresco", 40, 15},
        {"Atun Suli", 25, 8},
        {"Sardina vencida", 30, 20}
    };
    catalogo["bebidas"] = {
        {"Powercito", 120, 10},
        {"Jetcito", 95, 5},
        {"Monstercito", 15, 50}
    };
    catalogo["pastas"] = {
        {"Pasta de palito", 60, 12},
        {"Pasta de caracolito", 20, 30},
        {"Pasta gruesa", 10, 40}
    };
}

Producto* ServidorProductos::buscarProducto(const std::string& nombre){
    for (auto& categoria : catalogo) {
        for (auto& p : categoria.second) {
            if (p.nombre == nombre) return &p;
        }
    }
    return nullptr;
}

void ServidorProductos::waiting(){
    myMessage msg;
    while(running){
        buzon->Recibir(msg, SERVIDOR_PRODUCTOS);
        if(msg.st==REQUEST){
            std::cout<<"[SERVIDOR] solicitud recibida:"<<msg.message<<std::endl;
            procesarSolicitud(msg);
        }else if(msg.st==CLOSE){
            std::cout<<"[SERVIDOP] cerrando servidor:"<<std::endl;
            running=false;
        }
    }
}

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


    myMessage resp;//Nuevo mensaje para respuesta del servidpr
    resp.type=SERVIDOR_INTERMEDIO; //Para el intermediarip
    resp.st=RESPONSE; //el mensaje es una respuesta
    strncpy(resp.message, respuesta.c_str(), sizeof(resp.message) - 1); //copiatr al arreglo
    resp.message[sizeof(resp.message)-1] = '\0';//tama;o del arreglo
    buzon->Enviar(resp);
    std::cout << "[SERVIDOR] Enviando informacion al intermediario" << std::endl;
}