#ifndef SECUNDARIA_HPP
#define SECUNDARIA_HPP

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <exception>
#include <string>
#include <vector>
#include "Cliente.hpp"
#include "Producto.hpp"
#include "Carrito.hpp"

extern const std::string RUTA_LISTA;
extern const std::string RUTA_PROFORMA;
extern const std::string RUTA_SERVIDORES;
extern const std::string RUTA_RUTAS;
extern const std::string RUTA_BITACORA;

//Entrada de consola
std::string LeerLinea( const std::string & prompt ) ;
bool LeerEntero( const std::string & prompt, int & valor );

//Utilidades HTML
bool SiguienteEtiqueta( const std::string & html, const std::string & tag,size_t & pos, std::string & texto );
std::string AMinusculas( std::string s );
std::vector<std::string> DividirLineas( const std::string & texto );
std::string DecodificarEntidades( std::string s );
std::string HtmlATexto( const std::string & html );
bool SolicitudNoProcesada( const std::string & cuerpo );
void MostrarError( const std::string & contexto, const std::exception & e );
std::string ProformaATexto( const std::string & html );
bool SiguienteEtiqueta( const std::string & html, const std::string & tag, size_t & pos, std::string & texto );
std::string ProformaATexto( const std::string & html );
std::vector<std::string> parsearCategorias( const std::string & body );
std::string ArmarCuerpoProforma( const std::vector<ItemCarrito> & items );

//Consultas al intermediario
bool ObtenerCategorias( Cliente & cliente, const char * host, const char * servicio,std::vector<std::string> & categorias );
void MostrarCategorias( const std::vector<std::string> & categorias );
bool ObtenerProductos( Cliente & cliente, const char * host, const char * servicio,const std::string & categoria, std::vector<Producto> & productos );
void MostrarProductos( const std::vector<Producto> & productos );
void ConsultarMostrar( Cliente & cliente, const char * host, const char * servicio, const std::string & ruta, const std::string & titulo );

void ExtraerEventosNuevos( const std::string & html, long & ultimo_id, std::vector<std::string> & nuevos );

#endif // SECUNDARIA_HPP