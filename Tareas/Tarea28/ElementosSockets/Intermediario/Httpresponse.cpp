#include "Httpresponse.hpp"

std::string HttpResponse::Construir( const RespuestaServicio & r ) {
   // Content-Length en BYTES, no en caracteres -- importante porque el
   // HTML puede traer tildes/enies en UTF-8 (multi-byte).
   std::string encabezados;
   encabezados += "HTTP/1.1 " + std::to_string( r.codigo ) + " " + r.frase + "\r\n";
   encabezados += "Content-Type: " + r.tipoContenido + "\r\n";
   encabezados += "Content-Length: " + std::to_string( r.cuerpo.size() ) + "\r\n";
   encabezados += "Connection: close\r\n";   // simplifica: una conexion, un request, se cierra
   encabezados += "\r\n";

   return encabezados + r.cuerpo;
}