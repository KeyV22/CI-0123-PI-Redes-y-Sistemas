#include "Secundaria.hpp"

// Rutas HTTP del intermediario.
const std::string RUTA_LISTA = "/TicAmazon/list.php";
const std::string RUTA_PROFORMA = "/TicAmazon/proforma.php";
const std::string RUTA_SERVIDORES = "/TicAmazon/servidores";
const std::string RUTA_RUTAS = "/TicAmazon/rutas";
const std::string RUTA_BITACORA = "/TicAmazon/bitacora";

static const char * FRASES_NO_PROCESADA[] = {
   "no puede ser procesada",
   "no puede procesarse",
   "no esta disponible en el sistema"
};

std::string LeerLinea( const std::string & prompt ) {
   std::cout << prompt;
   std::string s;
   std::getline( std::cin, s );
   return s;
}

bool LeerEntero( const std::string & prompt, int & valor ) {
   std::string s = LeerLinea( prompt );
   try {
      size_t usados = 0;
      int v = std::stoi( s, &usados );
      if ( usados != s.size() ) return false;
      valor = v;
      return true;
   } catch ( const std::exception & ) {
      return false;
   }
}

bool SiguienteEtiqueta( const std::string & html, const std::string & tag,size_t & pos, std::string & texto ) {
   size_t ini = html.find( "<" + tag, pos );
   if ( ini == std::string::npos ) return false;
   size_t finApertura = html.find( '>', ini );
   size_t cierre = html.find( "</" + tag + ">", finApertura );
   if ( finApertura == std::string::npos || cierre == std::string::npos ) return false;
   texto = html.substr( finApertura + 1, cierre - finApertura - 1 );
   pos = cierre + tag.size() + 3;
   return true;
}

//manejo de errores
std::string AMinusculas( std::string s ) {
   for ( size_t i = 0; i < s.size(); i++ ) {
      s[i] = (char)std::tolower( (unsigned char)s[i] );
   }
   return s;
}

std::vector<std::string> DividirLineas( const std::string & texto ) {
   std::vector<std::string> lineas;
   std::istringstream in( texto );
   std::string linea;
   while ( std::getline( in, linea ) ) {
      size_t ini = linea.find_first_not_of( " \t\r" );
      if ( ini == std::string::npos ) continue;  // linea vacia
      size_t fin = linea.find_last_not_of( " \t\r" );
      lineas.push_back( linea.substr( ini, fin - ini + 1 ) );
   }
   return lineas;
}

std::string DecodificarEntidades( std::string s ) {
   const char * pares[][2] = { {"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&#39;", "'"}, {"&amp;", "&"} };
   for ( auto & par : pares ) {
      const std::string desde = par[0];
      const std::string hacia = par[1];
      size_t pos = 0;
      while ( (pos = s.find( desde, pos )) != std::string::npos ) {
         s.replace( pos, desde.size(), hacia );
         pos += hacia.size();
      }
   }
   return s;
}

std::string HtmlATexto( const std::string & html ) {
   std::string out;
   size_t i = 0;
   while ( i < html.size() ) {
      if ( html[i] != '<' ) {
         out += html[i++];
         continue;
      }
      size_t fin = html.find( '>', i );
      if ( fin == std::string::npos ) break;
 
      std::string tag = AMinusculas( html.substr( i + 1, fin - i - 1 ) );
      size_t k = 0;
      bool cierre = false;
      if ( k < tag.size() && tag[k] == '/' ) {
         cierre = true;
         k++;
      }
      size_t ini_nombre = k;
      while ( k < tag.size() && std::isalnum( (unsigned char)tag[k] ) ) k++;
      std::string nombre = tag.substr( ini_nombre, k - ini_nombre );
 
      if ( nombre == "br" ) {
         out += '\n';
      } else if ( cierre && ( nombre == "tr" || nombre == "li" || nombre == "p"
                  || nombre == "h1" || nombre == "h2" || nombre == "h3" || nombre == "div" ) ) {
         out += '\n';
      } else if ( cierre && ( nombre == "td" || nombre == "th" ) ) {
         out += "\t";
      }
      i = fin + 1;
   }
 
   std::ostringstream limpio;
   for ( const std::string & linea : DividirLineas( out ) ) {
      limpio << linea << "\n";
   }
   return limpio.str();
}

bool SolicitudNoProcesada( const std::string & cuerpo ) {
   std::string bajo = AMinusculas( cuerpo );
   for ( const char * frase : FRASES_NO_PROCESADA ) {
      if ( bajo.find( frase ) != std::string::npos ) return true;
   }
   return false;
}

void MostrarError( const std::string & contexto, const std::exception & e ) {
   if ( SolicitudNoProcesada( e.what() ) ) {
      std::cout << "La solicitud no puede ser procesada: el producto o la categoria no esta disponible en el sistema.\n";
   } else {
      std::cout << contexto << ": " << HtmlATexto( e.what() );
   }
}

// Convierte la pagina de la proforma en texto legible
std::string ProformaATexto( const std::string & html ) {
   std::ostringstream out;
   if ( html.find( "<TABLE" ) == std::string::npos ) {
      size_t pos = 0;
      std::string li;
      out << "No se pudo generar la proforma:\n";
      while ( SiguienteEtiqueta( html, "LI", pos, li ) ) {
         out << "  - " << li << "\n";
      }
      return out.str();
   }

   size_t posTabla = html.find( "<TABLE" );
   size_t finTabla = html.find( "</TABLE>", posTabla );
   std::string tabla = html.substr( posTabla, finTabla - posTabla );

   bool primera = true;
   size_t pos = 0;
   std::string fila;
   while ( SiguienteEtiqueta( tabla, "TR", pos, fila ) ) {
      std::vector<std::string> celdas;
      size_t p = 0;
      std::string celda;
      const std::string tag = primera ? "TH" : "TD";
      while ( SiguienteEtiqueta( fila, tag, p, celda ) ) celdas.push_back( celda );
      if ( celdas.size() < 6 ) continue;

      // Bodega, Categoria, Descripcion, Cantidad, Precio, Subtotal
      char linea[256];
      snprintf( linea, sizeof( linea ), "%-10s %-20s %-15s %8s %10s %10s\n",
                celdas[0].c_str(), celdas[1].c_str(), celdas[2].c_str(),
                celdas[3].c_str(), celdas[4].c_str(), celdas[5].c_str() );
      out << linea;
      primera = false;
   }

   size_t posTotal = html.find( "TOTAL:" );
   if ( posTotal != std::string::npos ) {
      size_t finTotal = html.find( '<', posTotal );
      out << html.substr( posTotal, finTotal - posTotal ) << "\n";
   }
   return out.str();
}

std::vector<std::string> parsearCategorias( const std::string & body ) {
   std::vector<std::string> categorias;
   size_t pos = 0;
   while ( (pos = body.find("<A href=", pos)) != std::string::npos ) {
      size_t inicioTexto = body.find('>', pos);
      if ( inicioTexto == std::string::npos ) break;
      size_t finTexto = body.find("</A>", inicioTexto);
      if ( finTexto == std::string::npos ) break;
      categorias.push_back( body.substr(inicioTexto + 1, finTexto - inicioTexto - 1) );
      pos = finTexto + 4;
   }
   return categorias;
}

// Arma el body application/x-www-form-urlencoded que espera
// POST /TicAmazon/proforma.php: item0_categoria=...&item0_producto=...&item0_cantidad...
std::string ArmarCuerpoProforma( const std::vector<ItemCarrito> & items ) {
   std::ostringstream cuerpo;
   for ( size_t i = 0; i < items.size(); ++i ) {
      if ( i > 0 ) cuerpo << "&";
      cuerpo << "item" << i << "_categoria=" << UrlEncode(items[i].producto.categoria)
             << "&item" << i << "_producto=" << UrlEncode(items[i].producto.descripcion)
             << "&item" << i << "_cantidad=" << items[i].cantidadElegida;
   }
   return cuerpo.str();
}

bool ObtenerCategorias( Cliente & cliente, const char * host, const char * servicio,std::vector<std::string> & categorias ) {
   try {
      categorias = parsearCategorias( cliente.Get( host, servicio, "/TicAmazon/list.php" ) );
   } catch ( const std::runtime_error & e ) {
      std::cout << "Error al consultar categorias: " << e.what() << "\n";
      return false;
   }
   if ( categorias.empty() ) {
      std::cout << "El servidor no tiene categorias disponibles.\n";
      return false;
   }
   return true;
}

void MostrarCategorias( const std::vector<std::string> & categorias ) {
   std::cout << "\nCategorias disponibles:\n";
   for ( size_t i = 0; i < categorias.size(); i++ ) {
      std::cout << "  [" << i << "] " << categorias[i] << "\n";
   }
}

bool ObtenerProductos( Cliente & cliente, const char * host, const char * servicio,const std::string & categoria, std::vector<Producto> & productos ) {
   try {
      std::string path = "/TicAmazon/list.php?category=" + UrlEncode( categoria );
      productos = parsearProductos( cliente.Get( host, servicio, path ) );
      return true;
   } catch ( const std::exception & e ) {
      std::cout << "Error al consultar productos: " << e.what() << "\n";
      return false;
   }
}

void MostrarProductos( const std::vector<Producto> & productos ) {
   for ( size_t i = 0; i < productos.size(); i++ ) {
      printf( "[%zu] %s - stock: %d - precio: %.2f (bodega: %s)\n",i, productos[i].descripcion.c_str(), productos[i].cantidad,productos[i].precio, productos[i].bodega.c_str() );
   }
}

void ConsultarMostrar( Cliente & cliente, const char * host, const char * servicio, const std::string & ruta, const std::string & titulo ) {
   try {
      std::string cuerpo = cliente.Get( host, servicio, ruta );
      if ( SolicitudNoProcesada( cuerpo ) ) {
         std::cout << "La solicitud no puede ser procesada por el intermediario.\n";
         return;
      }
      std::string texto = HtmlATexto( cuerpo );
      std::cout << "\n" << titulo << ":\n";
      if ( texto.empty() ) {
         std::cout << "  (sin datos)\n";
      } else {
         std::cout << texto;
      }
   } catch ( const std::exception & e ) {
      MostrarError( "Error al consultar " + titulo, e );
   }
}

void ExtraerEventosNuevos( const std::string & html, long & ultimo_id, std::vector<std::string> & nuevos ) {
   std::vector< std::pair<long, std::string> > eventos;
   size_t pos = 0;
   std::string fila;
   while ( SiguienteEtiqueta( html, "TR", pos, fila ) ) {
      std::vector<std::string> celdas;
      size_t p = 0;
      std::string celda;
      while ( SiguienteEtiqueta( fila, "TD", p, celda ) ) celdas.push_back( celda );
      if ( celdas.size() < 5 ) continue;
 
      long id = 0;
      try {
         size_t usados = 0;
         id = std::stol( celdas[0], &usados );
         if ( usados != celdas[0].size() ) continue;
      } catch ( const std::exception & ) {
         continue;
      }
      if ( id <= ultimo_id ) continue;
 
      eventos.push_back( std::make_pair( id, "#" + celdas[0] + "  " + celdas[1] + "  " + celdas[2] + "  "
                         + DecodificarEntidades( celdas[3] ) + "  " + DecodificarEntidades( celdas[4] ) ) );
   }
 
   std::sort( eventos.begin(), eventos.end() );
   for ( const auto & e : eventos ) {
      nuevos.push_back( e.second );
      ultimo_id = e.first;
   }
}