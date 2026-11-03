#ifndef HttpCliente_hpp
#define HttpCliente_hpp

#include <string>

struct RespuestaHttp {
   bool ok = false;          // false si no se pudo conectar / leer
   int codigo = 0;
   std::string cuerpo;
   std::string error;
};

// Hace UN request HTTP a un servidor de productos y devuelve la respuesta.
// Es el unico punto donde el intermediario envia/recibe hacia los servidores
// de productos (aqui se deberia poner el cifrado mas adelante).
class HttpCliente {
   public:
      static RespuestaHttp Enviar( const std::string & host, int puerto,const std::string & metodo, const std::string & ruta,const std::string & cuerpo = "", int timeoutSeg = 5 );
};

#endif