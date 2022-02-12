/*
 ============================================================================
 Name        : Broker
 Author      : Fran and Co
 Description : Buddy System Algorithm Header
 ============================================================================
 */

#ifndef BUDDY_SYSTEM_H_
#define BUDDY_SYSTEM_H_

#include "memory_commons.h"

typedef struct
{
	uint32_t id_data;
	void* data;
	int is_free;
	int base;
	int size;
	op_code queue;
} t_buddy;

void buddy_init();
void* buddy_alloc(int size);
void create_buddies(t_buddy* buddy, int size);
void associate_buddies(t_buddy* buddy);
t_buddy* find_free_smaller_buddy_to_hold(int size);
t_buddy* choose_victim_buddy();
int get_index_of_buddy_by_base(t_list* buddies, uint32_t base_buddy);
t_buddy* fifo_find_victim_buddy();
t_buddy* lru_find_victim_buddy();
void* find_data_buddy_by_id(uint32_t id);
t_buddy* find_buddy_by_id(t_list* list, uint32_t id);
unsigned upper_power_of_two(unsigned size);
int is_power_of_two(int number);

#endif /* BUDDY_SYSTEM_H_ */
