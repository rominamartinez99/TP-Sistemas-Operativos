/*
 ============================================================================
 Name        : Broker
 Author      : Fran and Co
 Description : Memory Header
 ============================================================================
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include <time.h>

#include "dynamic_partitions.h"
#include "buddy_system.h"

typedef struct
{
	void* alloc;
	void* data;
	uint32_t data_size;
	uint32_t id;
	op_code queue;
} t_copy_args;

// Mutex
extern pthread_mutex_t mutex_memory;

void load_memory(int size, int min_partition_size, int frequency, t_memory_algorithm memory_alg, t_selection_algorithm victim_alg, t_selection_algorithm partition_alg, char* dump_path);
void* memory_alloc(int size);
void* memory_copy(t_copy_args* args);
void* memory_get(uint32_t id);
t_list* get_victim_messages_ids(int* element_count);
void notify_all_victim_messages_deleted();
void notify_message_used(uint32_t id_message);
void memory_dump();
void end_memory();

#endif /* MEMORY_H_ */
