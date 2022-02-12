/*
 ============================================================================
 Name        : Broker
 Author      : Fran and Co
 Description : Dynamic Partitions Algorithm Header
 ============================================================================
 */

#ifndef DYNAMIC_PARTITIONS_H_
#define DYNAMIC_PARTITIONS_H_

#include "memory_commons.h"

typedef struct
{
	uint32_t id_data;
	void* data;
	int is_free;
	int base;
	int size;
	op_code queue;
} t_partition;

int SEARCH_FAILURE_COUNTER;

void dp_init();
void* dp_alloc(int size);
void adjust_partition_size(t_partition* partition, int size);
t_partition* find_free_partition(int size);
t_partition* choose_victim_partition();
void consolidate_free_partitions(t_partition* new_free_partition, int* partition_index_free);
int compact_memory();
int get_index_of_partition_by_base(t_list* partitions, uint32_t base_partition);
void* find_data_partition_by_id(uint32_t id);
t_partition* find_partition_by_id(t_list* list, uint32_t id);

t_partition* first_fit_find_free_partition(int size);
t_partition* best_fit_find_free_partition(int size);
t_partition* fifo_find_victim_partition();
t_partition* lru_find_victim_partition();

#endif /* DYNAMIC_PARTITIONS_H_ */
