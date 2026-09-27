/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *
  *  TicAmazon - Protocolo mancomunado del equipo
  *
  *  Implementa el formato de mensaje definido en el documento del equipo:
  *
  *     ORIGEN|DESTINO/TIPO_MENSAJE/campo1/campo2/...
  *
  *  NOTA IMPORTANTE sobre el documento fuente: las tablas resumen (secciones
  *  a/b/c) y los flujos detallados con ejemplos (secciones numeradas 1-11)
  *  no coinciden entre si en varios codigos (por ejemplo la tabla de la
  *  seccion b dice que "solicitar productos" es el codigo 20, pero el flujo
  *  #1 usa el codigo 21 para lo mismo; la tabla de Intermediario->Cliente
  *  dice que la lista de categorias es el codigo 11, pero el ejemplo usa 22
  *  tanto para Bodega->Intermediario como para Intermediario->Cliente).
  *  Esta implementacion sigue los FLUJOS DETALLADOS CON EJEMPLOS como fuente
  *  de verdad (son mas completos y especificos), no las tablas resumen.
  *
 **/

#ifndef PROTOCOLO_HPP
#define PROTOCOLO_HPP

#include <string>
#include <vector>
#include <sstream>

// ===== Codigos de tipo de mensaje (segun los flujos detallados del documento) =====

namespace TipoMensaje {
   const int REQUEST_CATEGORY   = 10;  // Cliente->Intermediario, o Intermediario->Intermediario (sin campos)
   const int CATEGORY_LIST_INT  = 11;  // Intermediario->Intermediario: lista de categorias del equipo remoto
   const int REQUEST_PRODUCTS_C = 20;  // Cliente->Intermediario: nombre_producto (detalle) / Intermediario->Intermediario: categoria
   const int REQUEST_PRODUCTS_B = 21;  // Intermediario->Bodega: solicita el listado de una categoria
   const int PRODUCT_LIST_B     = 22;  // Bodega->Intermediario, e Intermediario->Cliente: lista de productos de una categoria
   const int NO_PRODUCTS        = 23;  // Intermediario->Cliente: la categoria no tiene productos
   const int REQUEST_DETAIL_B   = 24;  // Intermediario->Bodega: pide el detalle de un producto puntual
   const int PRODUCT_DETAIL     = 25;  // Bodega->Intermediario: detalle de un producto (nombre/precio/stock/descripcion)
   const int PRODUCT_LIST_C     = 26;  // Intermediario->Cliente: lista de productos de una categoria (detalle, flujo 3)
   const int PRODUCT_NOT_FOUND  = 27;  // Bodega->Intermediario: el producto no existe
   const int PRODUCT_NOT_FOUND2 = 28;  // Intermediario->Cliente: el producto no existe
   const int ADD_TO_CART        = 30;  // Cliente->Intermediario: agregar producto al carrito
   const int CART_OK            = 32;  // Intermediario->Cliente: confirmacion de que se agrego al carrito
   const int CART_NO_STOCK      = 33;  // Intermediario->Cliente: no hay stock suficiente
   const int REQUEST_FACTURA    = 40;  // Cliente->Intermediario: pide el cierre de la orden (con el detalle del carrito)
   const int FACTURA            = 41;  // Intermediario->Cliente: factura proforma
   const int CARRITO_VACIO      = 42;  // Intermediario->Cliente: el carrito esta vacio
   const int RESERVE_STOCK      = 50;  // Intermediario->Bodega, o Intermediario->Intermediario: reserva stock
   const int ANNOUNCE           = 60;  // Anuncio de presencia (descubrimiento)
   const int DEATH              = 61;  // Anuncio de cierre ordenado
   const int ERROR_FORMATO      = 90;  // Campo no cumple regex
   const int ERROR_TAMANO       = 91;  // Campo fuera de los limites de tamano
   const int ERROR_COMUNICACION = 92;  // El otro extremo no respondio
}


struct MensajeV2 {
   std::string origen;
   std::string destino;
   int tipo = -1;
   std::vector<std::string> campos;
};


/**
  *  construirMensaje
  *     Arma la linea de texto del protocolo: ORIGEN|DESTINO/TIPO/campo1/campo2/...
  *     El tipo se imprime siempre con 2 digitos (ej: 5 -> "05"), tal como
  *     exige la regex del documento (^[0-9]{2}$).
  *
 **/
std::string construirMensaje( const std::string & origen, const std::string & destino,
                               int tipo, const std::vector<std::string> & campos = {} );

/**
  *  parsearMensaje
  *     Interpreta una linea recibida y la separa en sus componentes. No
  *     valida los campos (para eso esta ValidarMensaje); solo separa por
  *     los delimitadores "|" y "/".
  *
 **/
MensajeV2 parsearMensaje( const std::string & linea );


// ===== Validacion de campos individuales segun las expresiones regulares del documento =====

bool ValidarOrigenDestino( const std::string & valor );
bool ValidarTipoMensaje( const std::string & valor );
bool ValidarCategoria( const std::string & valor );
bool ValidarProducto( const std::string & valor );
bool ValidarPrecio( const std::string & valor );
bool ValidarStock( const std::string & valor );
bool ValidarCount( const std::string & valor );

// Limites de tamano (Tam_Min/Tam_Max) de la tabla del documento, para el error 91
bool TamanoValido( const std::string & campoTipo, const std::string & valor, int * tamMinOut = nullptr, int * tamMaxOut = nullptr );


/**
  *  ResultadoValidacion
  *     Resultado de validar un mensaje ya parseado contra las reglas del
  *     protocolo (formato + tamano). "ok" es false si algun campo fallo; en
  *     ese caso los demas campos indican cual fallo y por que, para poder
  *     construir el mensaje de error 90 o 91 correspondiente.
  *
 **/
struct ResultadoValidacion {
   bool ok = true;
   bool esErrorTamano = false;         // false = error de formato (90), true = error de tamano (91)
   std::string campoInvalido;
   std::string valorRecibido;
   int tamanoRecibido = 0;
   int tamanoMaximo = 0;
};

/**
  *  ValidarCampoPorTipo
  *     Valida un valor de campo segun su tipo semantico ("categoria",
  *     "producto", "precio", "stock", "descripcion", "count",
  *     "origen_destino", "tipo_mensaje"). Revisa primero el tamano y luego
  *     la expresion regular, llenando "resultado" si algo falla.
  *
 **/
bool ValidarCampoPorTipo( const std::string & tipoCampo, const std::string & valor, ResultadoValidacion * resultado );


// ===== Mensajes de error (seccion "Errores" del documento) =====

std::string construirError90( const std::string & detector, const std::string & sender,
                               int tipoMensajeOriginal, const std::string & campoInvalido,
                               const std::string & valorRecibido );

std::string construirError91( const std::string & detector, const std::string & sender,
                               int tipoMensajeOriginal, const std::string & campo,
                               int tamanoRecibido, int tamanoMaximo );

std::string construirError92( const std::string & detector, const std::string & sender,
                               const std::string & componenteNoResponde,
                               int tipoMensajePendiente, int intentos );

// Tamano maximo total de un mensaje, segun el documento (256 bytes)
const size_t TAMANO_MAXIMO_MENSAJE = 256;

#endif
