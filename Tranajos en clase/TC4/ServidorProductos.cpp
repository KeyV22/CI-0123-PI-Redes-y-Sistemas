#include "ServidorProductos.hpp"
#include "ServidorIntermedio.hpp"
#include <cstring>
#include <iostream>
#include "Buzon.hpp"

ServidorProductos::ServidorProductos(Buzon* b){
    running=true;
    buzon=b;
}

ServidorProductos::~ServidorProductos(){

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
    std::string productos;
    std:: string cadena=msg.message;

    if(strcmp(msg.message, "categorias")==0){
        std::string categorias =
        "Categorias disponibles en TicAmazon :)\n"
        "- enlatados \n"
        "- bebidas \n"
        "- pastas \n";
        productos = categorias; 
    }else if(cadena.find("GET")==0){
        std::string categoria = cadena.substr(4);
        std::cout << "[SERVIDOR] Buscando productos de \"" << categoria << "\"" << std::endl;

         if (categoria == "enlatados") {
            productos =
            "Lata de refresco : 40\n"
            "Atun Suli : 25\n"
            "Sardina vencida : 30\n";
        } else if (categoria == "bebidas") {
           productos =
            "Powercito : 120\n"
            "Jetcito : 95\n"
            "Monstercito : 15\n";
        } else if (categoria == "pastas") {
            productos =
            "Pasta de palito : 60\n"
            "Pasta de caracolito : 20\n"
            "Pasta gruesa : 10\n";
        } else {
            productos = "La categoria ingresada no es valida!\n";
        }
    } else {
        productos = "Comando Invalido!\n";
    }

    myMessage resp;//Nuevo mensaje para respuesta del servidpr
    resp.type=SERVIDOR_INTERMEDIO; //Para el intermediarip
    resp.st=RESPONSE; //el mensaje es una respuesta
    strncpy(resp.message, productos.c_str(), sizeof(resp.message) - 1); //copiatr al arreglo
    resp.message[sizeof(resp.message)-1] = '\0';//tama;o del arreglo
    buzon->Enviar(resp);
    std::cout << "[SERVIDOR] Enviando informacion al intermediario" << std::endl;
}