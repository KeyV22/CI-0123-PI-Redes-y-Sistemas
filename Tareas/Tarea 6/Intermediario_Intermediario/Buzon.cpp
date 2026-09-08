#include "Buzon.hpp"

Buzon::Buzon(){
    // Ya no se necesita crear una cola de kernel (msgget); el "buzon" vive en
    // memoria del propio proceso, en el map protegido por mutex.
}

Buzon::~Buzon(){
    // Nada que liberar manualmente: los containers de la STL se destruyen solos.
}

void Buzon::Enviar(const myMessage& msg){
    {
        std::lock_guard<std::mutex> lock(mtx);
        buzones[msg.type].push(msg);
    }
    cv.notify_all(); // despierta a cualquier hilo que este esperando en Recibir()
}

void Buzon::Recibir(myMessage& msg, long tipo){
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this, tipo] { return !buzones[tipo].empty(); });
    msg = buzones[tipo].front();
    buzones[tipo].pop();
}
