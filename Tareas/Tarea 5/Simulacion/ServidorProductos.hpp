class ServidorIntermedio;//declaraciones adelantadas
struct myMessage;
class Buzon;

class ServidorProductos{
    public:
        ServidorProductos(Buzon* b);
        ~ServidorProductos();
        void waiting();
        void procesarSolicitud(myMessage msg);
    
    private:
        bool running;
        Buzon* buzon;
};