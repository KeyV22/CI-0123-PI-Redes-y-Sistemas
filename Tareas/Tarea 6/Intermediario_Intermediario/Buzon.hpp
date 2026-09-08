#pragma once
#include <string.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <queue>
#include <map>
#include <mutex>
#include <condition_variable>

#define MaxParticipantes 100

enum states{
    CLIENTE=1,
    SERVIDOR_PRODUCTOS,
    SERVIDOR_INTERMEDIO,
    REQUEST,    //Pregunta
    RESPONSE, //Respuesta
    CLOSE,
    SERVIDOR_PRODUCTOS_B,     // bodega del segundo intermediario 
    SERVIDOR_INTERMEDIO_B     // buzon del segundo intermediario 
};

struct myMessage{
    long type;
    states st;
    char message[256];
};

// Buzon ahora es una "cola de mensajes" en memoria compartida entre hilos del
// mismo proceso (antes era una cola de mensajes de System V entre procesos
// creados con fork()). La interfaz publica (Enviar/Recibir) se mantiene igual
// para que ServidorProductos, ServidorIntermedio y simulacion.cpp no tengan
// que cambiar su forma de usarla.
class Buzon{
    public:
        Buzon();
        ~Buzon();

        void Enviar(const myMessage& msg);
        void Recibir(myMessage& msg, long type);

    private:
        std::map<long, std::queue<myMessage>> buzones; // un sub-buzon por cada "type" destinatario
        std::mutex mtx;
        std::condition_variable cv;
};
