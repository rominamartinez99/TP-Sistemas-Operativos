/*
 * planificador.h
 *
 *  Created on: 15 jun. 2020
 *      Author: utnso
 */

#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<string.h>
#include "nuestras-commons/conexion.h"
#include "nuestras-commons/mensajes.h"

#include "logger.h"

#include<pthread.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>

#include<semaphore.h>

char* algoritmoPlanificacion;
int quantum;
int estimacionInicial;
double alfa;
int retardoCPU;
int cantidadDeadlocksResueltos;
int cantidadCambiosDeContexto;

int inicioAlgoritmoDeadlock;

t_config* config;

//Listas de entrenadores segun estado
t_list* listaNuevos;
t_list* listaReady;
t_list* listaBloqueadosDeadlock;
t_list* listaBloqueadosEsperandoMensaje;
t_list* listaBloqueadosEsperandoPokemones;
t_list* listaFinalizados;


//listas de pokemones
t_list* atrapados;
t_list* pendientes;
t_list* objetivoTeam;
t_list* entrenadores;

t_list* entrenadorIntercambio;
t_list* entrenadorConQuienIntercambiar;
t_list* entrenadoresNoSeleccionables;

//lista quantum
t_list* quantumPorEntrenador;

//Variables a liberar
t_list* socketsALiberar;


//semaforos
t_list* sem_entrenadores_ejecutar;
sem_t sem_planificar;
sem_t sem_esperarCaught;
sem_t sem_buscarEntrenadorMasCercano;
sem_t sem_entrenadorMoviendose;

extern pthread_mutex_t mutex_atrapados;
extern pthread_mutex_t mutex_pendientes;
extern pthread_mutex_t mutex_objetivoTeam;
extern pthread_mutex_t mutex_entrenadores;

extern pthread_mutex_t mutex_listaNuevos;
extern pthread_mutex_t mutex_listaReady;
extern pthread_mutex_t mutex_listaBloqueadosDeadlock;
extern pthread_mutex_t mutex_listaBloqueadosEsperandoMensaje;
extern pthread_mutex_t mutex_listaBloqueadosEsperandoPokemones;
extern pthread_mutex_t mutex_listaFinalizados;

extern pthread_mutex_t mutex_cantidadDeadlocks;
extern pthread_mutex_t mutex_cantidadCambiosContexto;

typedef enum{
	NEW = 1,
	READY = 2,
	BLOCKED = 3,
	EXEC = 4,
	FINISHED = 5
}status_code;

typedef enum{
	FIFO = 1,
	RR = 2,
	SJFCD = 3,
	SJFSD = 4,
	ERROR_CODIGO_ALGORITMO = 9

}algoritmo_code;

const static struct {
	algoritmo_code codigo_algoritmo;
	const char* str;
} conversionAlgoritmo[] = {
		{FIFO, "FIFO"},
		{RR, "RR"},
		{SJFSD, "SJF-SD"},
		{SJFCD, "SJF-CD"}

};

typedef struct
{
	t_nombrePokemon* pokemon;
	t_coordenadas* coordenadas;
} t_newPokemon;

typedef struct
{
	uint32_t id_entrenador;
	t_coordenadas* coordenadas;
	t_list* pokemonesQuePosee;
	t_list* pokemonesQueQuiere;
	uint32_t cantidad_pokemons;
	t_newPokemon* pokemonInstantaneo;
	status_code estado;
	uint32_t idMensajeCaught;
	uint32_t puedeAtrapar;
	uint32_t esLocalized;
	uint32_t misCiclosDeCPU;
	uint32_t quantumDisponible;
	uint32_t quantumIntercambio;
	double estimacionInicial;
	uint32_t rafagaAnteriorReal;
} t_entrenador;

void planificarCaught();

void planificarSegun();

void planificarSegunFifo();

void planificarSegunSJFSinDesalojo();

int planificarSegunRR();

int planificarSegunSJFConDesalojo();

void chequearSiEstaDisponible(t_entrenador*);

void chequearDeadlock(int);

algoritmo_code stringACodigoAlgoritmo(const char*);

int distanciaA(t_coordenadas*, t_coordenadas*);

int tieneTodoLoQueQuiere(t_entrenador*);

void diferenciaYCargarLista(t_list*, t_list*, t_list*);

int sonIguales(t_nombrePokemon*, t_nombrePokemon*);

void inicializarListasDeEstados();

void verificarTieneTodoLoQueQuiere(t_entrenador*);

void sacarEntrenadorDeLista(t_entrenador*, t_list*);

t_entrenador* elegirConQuienIntercambiar(t_entrenador*);

int tengoAlgunPokemonQueQuiere2(t_entrenador*,t_entrenador*);

void ordenarListaPorEstimacion(t_list*);

int llegoAlObjetivoPokemon(t_entrenador*);

int llegoAlObjetivoEntrenador(t_entrenador*, t_entrenador*);

int getIndexEntrenadorEnLista(t_list*, t_entrenador*);

void intercambiarPokemonesEntre(t_entrenador*, t_entrenador*);

void diferenciasListasDeadlock(t_list*, t_list*, t_list*);

void pokemonsQuePuedeDarle(t_list*, t_list*, t_list*);

void finalizarTeam();

#endif /* PLANIFICADOR_H_ */
