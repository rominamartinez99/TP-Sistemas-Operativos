/*
 ============================================================================
 Name        : Logger
 Author      : Fran and Co
 Description : Logger
 ============================================================================
 */

#include "logger.h"

void log_nuevo_mensaje(const char* nombre_cola, uint32_t id, uint32_t id_corr)
{
	char log_msg[100];
	sprintf(log_msg, "Se ha recibido un nuevo mensaje: CODIGO DE OPERACION: %s, ID: %d y ID CORRELATIVO: %d", nombre_cola, id, id_corr);
	log_info(LOGGER,log_msg);
}

void log_nueva_suscripcion(const char* nombre_cola)
{
	char log_msg[100];
	sprintf(log_msg, "Se ha realizado una suscripción a la cola: %s", nombre_cola);
	log_info(LOGGER,log_msg);
}

void log_conexion_a_proceso(char* ip, char* puerto)
{
	char log_msg[100];
	sprintf(log_msg, "Se ha realizado una conexión al proceso con: IP: %s, PUERTO: %s", ip, puerto);
	log_info(LOGGER,log_msg);
}

