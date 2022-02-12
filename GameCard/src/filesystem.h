/*
 * filesystem.h
 *
 *  Created on: 23 jun. 2020
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <pthread.h>
// INCLUDES FILESYSTEM
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
// INCLUDES LOCALES
#include "nuestras-commons/conexion.h"
#include "nuestras-commons/mensajes.h"

char* PUNTO_MONTAJE;
t_config* configGeneral;

void configuracionInicial(void);

void verificarDirectorio(char*);
char* verificarPokemon(char*);
void verificarMetadataPokemon(char*);
char* verificarBloque(t_config*);

void verificarMetadata(char*);
void verificarBitmap(char*);
void verificarBloquesIniciales(char*);

char* obtenerRutaTotal(char*);
char* obtenerRutaBloque(int);

int obtenerLugarEnBitmap();
char* armarArrayDeBloques(char**, int);

bool sePuedeEscribirElUltimoBloque(int);
bool existePokemon(char*);
void separarLinea(char*, int*);
char* leerBloque(char*);
int armarVectorCoordenadas(char**, int**);

t_config* abrirArchivo(char*);
void cerrarArchivo(t_config*);
char* asignarBloque(t_config*);
void escribirArchivoPokemon(char*, t_config*);
int escribirBloque(char*, char*);
int cantidadElementosArray(char**);
void liberarArray(char**);
void borrarBloques(t_config*);
char* armarArchivoPokemon(char**);
char* modificarCoordenada(char*, int);

t_appearedPokemon_msg procesarNewPokemon(t_newPokemon_msg*);
t_localizedPokemon_msg procesarGetPokemon(t_getPokemon_msg*);
t_caughtPokemon_msg procesarCatchPokemon(t_catchPokemon_msg*);
char* arreglarNombrePokemon(t_nombrePokemon);
void rearmarArchivosPokemon(char**, t_config*, int, int);

void obtenerCoordenadas(char**, int*);
int existeCoordenada(char**, char*);

#endif /* FILESYSTEM_H_ */
