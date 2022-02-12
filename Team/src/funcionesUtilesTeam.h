/*
 * funcionesUtilesTeam.h
 *
 *  Created on: 4 may. 2020
 *      Author: utnso
 */

#ifndef FUNCIONESUTILESTEAM_H_
#define FUNCIONESUTILESTEAM_H_

#include "planificador.h"

//Variables Globales
uint32_t ID_ENTRENADORES;

// Listas
t_list* hilosEntrenadores;
t_list* objetivoTeam;
t_list* atrapados;
t_list* pendientes;
t_list* id_mensajeGet;
t_list* id_mensajeCatch;
t_list* mensajesLocalized;
t_list* especiesRequeridas;
t_list* especiesQueLlegaron;

// Variables globales
char* IP_TEAM;
char* PUERTO_TEAM;
char* ipBroker;
char* puertoBroker;
int ID_TEAM;
int TIEMPO_RECONEXION;

// Mutexs
extern pthread_mutex_t mutex_id_entrenadores;
extern pthread_mutex_t mutex_entrenador;
extern pthread_mutex_t mutex_hay_pokemones;

extern pthread_mutex_t mutex_send;

extern pthread_mutex_t mutex_id_mensaje_get;
extern pthread_mutex_t mutex_id_mensaje_catch;
extern pthread_mutex_t mutex_mensajesLocalized;

extern pthread_mutex_t mutex_especies_requeridas;
extern pthread_mutex_t mutex_especies_que_llegaron;

t_list* organizarPokemones(char**);

void ponerEntrenadoresEnLista();

void crearHilosEntrenadores();

t_entrenador* crear_entrenador(uint32_t, t_coordenadas*, t_list*, t_list*, status_code);

t_list* organizarPokemones(char**);

t_nombrePokemon* crear_pokemon(char*);

uint32_t generar_id();

void ejecutarEntrenador(t_entrenador*);

uint32_t enviarMensajeCatch(t_newPokemon*);

uint32_t esperarIdCatch(int);

void moverAlEntrenadorHastaUnPokemon(uint32_t);

void evaluarEstadoPrevioAAtrapar(t_entrenador*);

void atraparPokemon(t_entrenador*);

void hacerObjetivoTeam(t_list*,t_list*);

void aplanarDobleLista(t_list*);

t_entrenador* entrenadorMasCercano(t_newPokemon*);

void buscarPokemonAppeared(t_newPokemon*);

void buscarPokemonLocalized(t_localizedPokemon_msg*, uint32_t);

void ponerEntrenadorEnReady(t_entrenador*, t_newPokemon*);

void moverAlEntrenadorHastaOtroEntrenador(uint32_t, uint32_t);

void intercambiarPokemones(uint32_t, uint32_t);

void dameTuPokemon(t_entrenador*, t_entrenador*);

void sacarPokemonDe(t_nombrePokemon*, t_list*);

#endif /* FUNCIONESUTILESTEAM_H_ */
