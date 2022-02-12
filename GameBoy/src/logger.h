/*
 ============================================================================
 Name        : Logger
 Author      : Fran and Co
 Description : Header Logger
 ============================================================================
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include<stdint.h>
#include<commons/log.h>

t_log* LOGGER;

void log_nuevo_mensaje(const char* nombre_cola, uint32_t id, uint32_t id_corr);
void log_nueva_suscripcion(const char* nombre_cola);
void log_conexion_a_proceso(char* ip, char* puerto);

#endif /* LOGGER_H_ */
