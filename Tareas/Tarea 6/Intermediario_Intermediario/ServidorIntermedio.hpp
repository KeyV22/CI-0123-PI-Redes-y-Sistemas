#pragma once
#include <string>
#include <vector>
#include <map>

class Cliente;
class ServidorProductos;
struct myMessage;
class Buzon;

struct ItemCarrito{
    std::string nombre;
    int cantidad;
    int precio;
};

class ServidorIntermedio {
    public:
        // miTipo:        buzon propio de este intermediario (SERVIDOR_INTERMEDIO o _B)
        // tipoMiBodega:  buzon de SU bodega local (SERVIDOR_PRODUCTOS o _B)
        // tipoPeer:      buzon del intermediario par a quien preguntarle (ROUTE_CAT)
        // miId/idPeer:   nombres para los mensajes PING_PEER/PONG_PEER y logging
        ServidorIntermedio(Buzon* b, long miTipo, long tipoMiBodega, long tipoPeer,
                            std::string miId, std::string idPeer, std::string nombreLog);
        void connect(ServidorProductos* servp);
        ~ServidorIntermedio();
        void waiting();

    private:
        Buzon* buzon;
        ServidorProductos* serv;

        long miTipo;
        long tipoMiBodega;
        long tipoPeer;
        std::string miId;
        std::string idPeer;
        std::string nombreLog;

        std::vector<ItemCarrito> carrito;
        std::map<std::string, int> precioCache;
        std::string ultimoComandoCliente;
        std::string nombrePendiente;
        int cantidadPendiente = 0;
        int siguienteIdOrden = 1;

        void cachearPrecios(const std::string& listaProd);

        // --- Atiende una solicitud que vino de un CLIENTE ---
        void atenderCliente(myMessage msg);
        // --- Atiende una solicitud que vino de un intermediario PAR (ROUTE_CAT / FWD_PROD / PING_PEER) ---
        void atenderPeer(myMessage msg);

        // Envia algo a mi bodega local y espera (sincrono) su respuesta
        std::string preguntarleAMiBodega(const std::string& mensaje);
        // Envia algo a mi peer y espera (sincrono) su respuesta
        std::string preguntarleAMiPeer(const std::string& mensaje);
};
