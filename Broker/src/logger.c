/*
 ============================================================================
 Name        : Logger
 Author      : Fran and Co
 Description : Logger
 ============================================================================
 */

#include "logger.h"

void log_new_connection(int socket_client)
{
	char log_msg[100];
	sprintf(log_msg, "El cliente socket: %d se ha conectado", socket_client);
	log_info(LOGGER,log_msg);
}

void log_new_subscriber(uint32_t id_subscriber, op_code queue)
{
	char* code = op_code_a_string(queue);
	char log_msg[100];
	sprintf(log_msg, "El proceso id: %d se ha suscripto a la cola: %s",id_subscriber, code);
	log_info(LOGGER,log_msg);
}

void log_new_message(uint32_t id_message, op_code queue)
{
	char* code = op_code_a_string(queue);
	char log_msg[100];
	sprintf(log_msg, "El mensaje id: %d ha llegado a la cola: %s", id_message, code);
	log_info(LOGGER,log_msg);
}

void log_message_to_subscriber(uint32_t id_subscriber, uint32_t id_message)
{
	char log_msg[100];
	sprintf(log_msg, "El mensaje id: %d ha sido enviado al suscriptor id: %d", id_message, id_subscriber);
	log_info(LOGGER,log_msg);
}

void log_ack_from_subscriber(uint32_t id_subscriber, uint32_t id_message)
{
	char log_msg[100];
	sprintf(log_msg, "El suscriptor id: %d ha recibido el mensaje id: %d", id_subscriber, id_message);
	log_info(LOGGER,log_msg);
}

void log_new_message_in_memory(uint32_t id_message, int partition_base)
{
	char log_msg[100];
	sprintf(log_msg, "El mensaje id: %d se ha guardado en memoria en la partición base: %d", id_message, partition_base);
	log_info(LOGGER,log_msg);
}

void log_deleted_partition(int partition_base)
{
	char log_msg[100];
	sprintf(log_msg, "La partición base: %d ha sido eliminada", partition_base);
	log_info(LOGGER,log_msg);
}

void log_compactation()
{
	char log_msg[100];
	sprintf(log_msg, "La memoria ha sido compactada");
	log_info(LOGGER,log_msg);
}

void log_buddy_association(int base_buddy_1, int base_buddy_2)
{
	char log_msg[100];
	sprintf(log_msg, "Los buddies con bases %d y %d se han consolidado", base_buddy_1, base_buddy_2);
	log_info(LOGGER,log_msg);
}

void log_dump()
{
	char log_msg[100];
	sprintf(log_msg, "La Dump de cache ha sido solicitada");
	log_info(LOGGER,log_msg);
}
