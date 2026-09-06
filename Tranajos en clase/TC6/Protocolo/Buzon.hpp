#include <sys/msg.h>
#include <string.h>
#include <cstring>
#include <unistd.h> //pid_t
#include <sys/types.h>
#include <iostream>
#include <sys/types.h>
#include <sys/ipc.h>

#define MaxParticipantes 100

enum states{
    CLIENTE=1,
    SERVIDOR_PRODUCTOS,
    SERVIDOR_INTERMEDIO,
    REQUEST,    //Pregunta
    RESPONSE, //Respuesta
    CLOSE
};

struct myMessage{
    long type;
    states st;
    char message[256];
};

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