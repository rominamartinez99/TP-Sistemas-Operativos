/*
 ============================================================================
 Name        : Broker
 Author      : Fran and Co
 Description : Memory Common Logic
 ============================================================================
 */

#include "memory_commons.h"

pthread_mutex_t mutex_lru_list = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_deleted_messages_ids = PTHREAD_MUTEX_INITIALIZER;

void* get_first(t_list* partitions)
{
	if (partitions->head == NULL)
		return NULL;

	return list_remove(partitions, 0);
}
