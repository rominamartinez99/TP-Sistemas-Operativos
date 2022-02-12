/*
 ============================================================================
 Name        : GameCard
 Author      : Fran and Co
 Description : Header Proceso GameCard
 ============================================================================
 */

#ifndef GAMECARD_H_
#define GAMECARD_H_

#include "filesystem.h"

// DEFINES
#define GAMECARD_CONFIG "gamecard.config"
#define GAMECARD_LOG "gamecard.log"

// THREADS
pthread_t threadNewPokemon;
pthread_t threadGetPokemon;
pthread_t threadCatchPokemon;
pthread_t threadMessages;

// STRUCTS
typedef struct {
	int id_hilo;
	op_code tipoCola;
} t_datosHilo;

void conectarseYSuscribirse(t_suscripcion_msg*);
t_config* setear_config(void);
void recepcionMensajesDeCola(t_suscripcion_msg*, int);
void devolverMensajeCorrespondiente(t_paquete*);
void esperarMensajes(void);
void serve_client(int*);
bool chequearMensajeBroker(int socketTemporal);

#endif /* GAMECARD_H_ */
