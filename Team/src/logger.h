/*
 * logger.h
 *
 *  Created on: 12 jun. 2020
 *      Author: utnso
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include<stdint.h>
#include<string.h>
#include<stdlib.h>
#include<commons/log.h>

t_log* LOGGER;

void log_entrenador_cambio_de_cola_planificacion(uint32_t, char*, char*);
void log_movimiento_entrenador(uint32_t, uint32_t, uint32_t);
void log_atrapo_al_pokemon(uint32_t, char*, uint32_t, uint32_t);
void log_intercambio_pokemones(uint32_t, uint32_t);
void log_inicio_algoritmo_deadlock();
void log_fin_algoritmo_deadlock(char*);
void log_llegada_appeared(uint32_t, char*, uint32_t, uint32_t);
void log_llegada_caught(uint32_t, uint32_t);
void log_llegada_localized(uint32_t, char*, uint32_t, char*);
void log_resultado_team(char* resultado, int, int, char*, int);
void log_error_comunicacion_con_broker();
void log_inicio_reintento_conexion_broker();
void log_resultado_proceso_reintento_conexion_broker(int);

#endif /* LOGGER_H_ */
