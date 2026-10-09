#ifndef HttpResponse_hpp
#define HttpResponse_hpp

#include <string>
#include "ServicioProductos.hpp"   // define RespuestaServicio

// Serializa un RespuestaServicio (codigo, frase, tipoContenido, cuerpo)
// al texto HTTP crudo que se escribe directamente al socket.
class HttpResponse {
   public:
      static std::string Construir( const RespuestaServicio & r );
};

#endif