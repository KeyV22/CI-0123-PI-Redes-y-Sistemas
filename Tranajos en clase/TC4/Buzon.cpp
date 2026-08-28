#include "Buzon.hpp"

Buzon::Buzon(){
    key_t key= 0xC4849
    this->id=msgget(key, IPC_CREAT|0600); //Devolver id
    if(this->id<0){ 
        throw std::runtime_error("ERROR, SE RECIBIO UN NEGATIVO");
    }

    this->owner=getpid(); //Para saber que proceso creo el buzon
}

Buzon::~Buzon(){
    // Como fork() copia este objeto a los procesos hijos, cada uno tendria su
    // propia copia y llamaria a este destructor por separado. Sin esto,
    // el primer proceso en terminar destruiria el buzon para todos los demas.
    if (getpid() == this->owner) {
        msgctl(this->id, IPC_RMID, NULL);//controlar cola de mensajes
    }
}

ssize_t Buzon::Enviar(const myMessage& msg){
    ssize_t result= msgsnd(this->id, &msg, sizeof(myMessage)-sizeof(long),0);//envia mensaje a la cola

    if(result<0){
        throw std::runtime_error("Error con el envio del mensaje");
    }
    return result
}

ssize_t Buzon::Recibir(myMessage& msg, long tipo=0){
    ssize_t r=msgrcv(this->id, &msg,sizeof(myMessage)-sizeof(long), tipo,0); //recibe mensaje de cola

    if(r<0){
        throw std::runtime_error("Error recibiendo el mensaji");
    }
    return r;
}