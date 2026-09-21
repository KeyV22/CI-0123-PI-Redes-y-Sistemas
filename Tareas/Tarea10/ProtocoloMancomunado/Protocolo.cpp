/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *
  *  TicAmazon - Protocolo mancomunado del equipo (implementacion)
  *
 **/

#include "Protocolo.hpp"
#include <regex>
#include <iomanip>

std::string construirMensaje( const std::string & origen, const std::string & destino,
                               int tipo, const std::vector<std::string> & campos ) {

   std::ostringstream out;
   out << origen << "|" << destino << "/" << std::setw( 2 ) << std::setfill( '0' ) << tipo;

   for ( auto & campo : campos ) {
      out << "/" << campo;
   }

   return out.str();

}


MensajeV2 parsearMensaje( const std::string & linea ) {

   MensajeV2 m;

   size_t posPipe = linea.find( '|' );
   if ( std::string::npos == posPipe ) {
      return m;   // mensaje mal formado; tipo queda en -1 para que el llamador lo detecte
   }
   m.origen = linea.substr( 0, posPipe );

   size_t posBarra1 = linea.find( '/', posPipe );
   if ( std::string::npos == posBarra1 ) {
      return m;
   }
   m.destino = linea.substr( posPipe + 1, posBarra1 - posPipe - 1 );

   size_t posBarra2 = linea.find( '/', posBarra1 + 1 );
   std::string tipoStr = ( std::string::npos == posBarra2 )
                        ? linea.substr( posBarra1 + 1 )
                        : linea.substr( posBarra1 + 1, posBarra2 - posBarra1 - 1 );

   try {
      m.tipo = std::stoi( tipoStr );
   } catch ( ... ) {
      m.tipo = -1;
      return m;
   }

   // separar el resto de los campos por "/"
   size_t pos = posBarra2;
   while ( std::string::npos != pos ) {
      size_t siguiente = linea.find( '/', pos + 1 );
      std::string campo = ( std::string::npos == siguiente )
                         ? linea.substr( pos + 1 )
                         : linea.substr( pos + 1, siguiente - pos - 1 );
      m.campos.push_back( campo );
      pos = siguiente;
   }

   return m;

}


// ===== Expresiones regulares =====

bool ValidarOrigenDestino( const std::string & valor ) {
   static const std::regex re( "^((CLI|INT)_[0-9]{2}|SERV)$" );
   return std::regex_match( valor, re );
}

bool ValidarTipoMensaje( const std::string & valor ) {
   static const std::regex re( "^[0-9]{2}$" );
   return std::regex_match( valor, re );
}

bool ValidarCategoria( const std::string & valor ) {
   static const std::regex re( "^[a-zA-Z]{3,20}$" );
   return std::regex_match( valor, re );
}

bool ValidarProducto( const std::string & valor ) {
   static const std::regex re( "^[a-zA-Z0-9\\-]{1,50}$" );
   return std::regex_match( valor, re );
}

bool ValidarPrecio( const std::string & valor ) {
   static const std::regex re( "^[0-9]{1,6}(\\.[0-9]{1,2})?$" );
   return std::regex_match( valor, re );
}

bool ValidarStock( const std::string & valor ) {
   static const std::regex re( "^[0-9]{1,5}$" );
   return std::regex_match( valor, re );
}

bool ValidarCount( const std::string & valor ) {
   static const std::regex re( "^[0-9]{1,3}$" );
   return std::regex_match( valor, re );
}


bool TamanoValido( const std::string & campoTipo, const std::string & valor, int * tamMinOut, int * tamMaxOut ) {

   int tamMin = 0, tamMax = 0;

   if ( "origen_destino" == campoTipo )   { tamMin = 4;  tamMax = 6;   }
   else if ( "tipo_mensaje" == campoTipo ) { tamMin = 2;  tamMax = 2;   }
   else if ( "categoria" == campoTipo )    { tamMin = 3;  tamMax = 20;  }
   else if ( "producto" == campoTipo )     { tamMin = 1;  tamMax = 50;  }
   else if ( "precio" == campoTipo )       { tamMin = 1;  tamMax = 8;   }
   else if ( "stock" == campoTipo )        { tamMin = 1;  tamMax = 5;   }
   else if ( "descripcion" == campoTipo )  { tamMin = 0;  tamMax = 100; }
   else if ( "count" == campoTipo )        { tamMin = 1;  tamMax = 3;   }
   else                                    { tamMin = 0;  tamMax = 999; }  // tipo desconocido: no restringe

   if ( nullptr != tamMinOut ) *tamMinOut = tamMin;
   if ( nullptr != tamMaxOut ) *tamMaxOut = tamMax;

   int len = (int) valor.size();
   return ( len >= tamMin && len <= tamMax );

}


bool ValidarCampoPorTipo( const std::string & tipoCampo, const std::string & valor, ResultadoValidacion * resultado ) {

   int tamMin, tamMax;

   if ( !TamanoValido( tipoCampo, valor, &tamMin, &tamMax ) ) {
      resultado->ok = false;
      resultado->esErrorTamano = true;
      resultado->campoInvalido = tipoCampo;
      resultado->valorRecibido = valor;
      resultado->tamanoRecibido = (int) valor.size();
      resultado->tamanoMaximo = tamMax;
      return false;
   }

   bool formatoOk = true;
   if ( "origen_destino" == tipoCampo )      formatoOk = ValidarOrigenDestino( valor );
   else if ( "tipo_mensaje" == tipoCampo )    formatoOk = ValidarTipoMensaje( valor );
   else if ( "categoria" == tipoCampo )       formatoOk = ValidarCategoria( valor );
   else if ( "producto" == tipoCampo )        formatoOk = ValidarProducto( valor );
   else if ( "precio" == tipoCampo )          formatoOk = ValidarPrecio( valor );
   else if ( "stock" == tipoCampo )           formatoOk = ValidarStock( valor );
   else if ( "count" == tipoCampo )           formatoOk = ValidarCount( valor );
   // "descripcion" no tiene regex propia en el documento: solo se valida el tamano

   if ( !formatoOk ) {
      resultado->ok = false;
      resultado->esErrorTamano = false;
      resultado->campoInvalido = tipoCampo;
      resultado->valorRecibido = valor;
      return false;
   }

   resultado->ok = true;
   return true;

}


std::string construirError90( const std::string & detector, const std::string & sender,
                               int tipoMensajeOriginal, const std::string & campoInvalido,
                               const std::string & valorRecibido ) {

   std::ostringstream tipoStr;
   tipoStr << std::setw( 2 ) << std::setfill( '0' ) << tipoMensajeOriginal;

   return construirMensaje( detector, sender, TipoMensaje::ERROR_FORMATO,
                             { tipoStr.str(), campoInvalido, valorRecibido } );

}

std::string construirError91( const std::string & detector, const std::string & sender,
                               int tipoMensajeOriginal, const std::string & campo,
                               int tamanoRecibido, int tamanoMaximo ) {

   std::ostringstream tipoStr;
   tipoStr << std::setw( 2 ) << std::setfill( '0' ) << tipoMensajeOriginal;

   return construirMensaje( detector, sender, TipoMensaje::ERROR_TAMANO,
                             { tipoStr.str(), campo, std::to_string( tamanoRecibido ), std::to_string( tamanoMaximo ) } );

}

std::string construirError92( const std::string & detector, const std::string & sender,
                               const std::string & componenteNoResponde,
                               int tipoMensajePendiente, int intentos ) {

   std::ostringstream tipoStr;
   tipoStr << std::setw( 2 ) << std::setfill( '0' ) << tipoMensajePendiente;

   return construirMensaje( detector, sender, TipoMensaje::ERROR_COMUNICACION,
                             { componenteNoResponde, tipoStr.str(), std::to_string( intentos ) } );

}
