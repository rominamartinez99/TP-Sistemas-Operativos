/*
 ============================================================================
 Name        : Broker
 Author      : Fran and Co
 Description : Header Memory Common Logic
 ============================================================================
 */

#ifndef MEMORY_COMMONS_H_
#define MEMORY_COMMONS_H_

#include <stdio.h>
#include <stddef.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <commons/collections/list.h>
#include "logger.h"


// Enums memorias
typedef enum {
	DYNAMIC_PARTITIONS,
	BUDDY_SYSTEM,
} t_memory_algorithm;

typedef enum {
	FIRST_FIT,
	BEST_FIT,
	FIFO,
	LRU,
	NONE
} t_selection_algorithm;

typedef struct
{
	uint32_t* id;
	op_code* queue;
} t_message_deleted;

// Constantes
void* MEMORY;
int MEMORY_SIZE;
int MIN_PARTITION_SIZE;
int COMPACTION_FREQUENCY;
t_memory_algorithm MEMORY_ALGORITHM;
t_selection_algorithm PARTITION_SELECTION_ALGORITHM;
t_selection_algorithm VICTIM_SELECTION_ALGORITHM;

char* DUMP_PATH;

t_list* lru_list;
t_list* deleted_messages_ids;
t_list* FREE_PARTITIONS;
t_list* OCCUPIED_PARTITIONS;
t_list* ALL_PARTITIONS;

// Mutex
extern pthread_mutex_t mutex_lru_list;

extern pthread_mutex_t mutex_deleted_messages_ids;

void* get_first(t_list* partitions);

#endif /* MEMORY_COMMONS_H_ */
