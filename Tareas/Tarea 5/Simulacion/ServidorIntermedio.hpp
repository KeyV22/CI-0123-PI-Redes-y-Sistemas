class Cliente;
class ServidorProductos;
struct Message;
class Buzon;

class ServidorIntermedio {
    public:
        ServidorIntermedio(Buzon* b);
        void connect(ServidorProductos* servp);
        ~ServidorIntermedio();
        void waiting();
    private:
        Buzon* buzon;
        ServidorProductos* serv;
};