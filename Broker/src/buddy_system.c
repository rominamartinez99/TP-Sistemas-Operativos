/*
 ============================================================================
 Name        : Broker
 Author      : Fran and Co
 Description : Buddy System Algorithm
 ============================================================================
 */

#include "buddy_system.h"

void buddy_init()
{
	t_buddy* initial_buddy = malloc(sizeof(*initial_buddy));
	initial_buddy->id_data = 0;
	initial_buddy->data = NULL;
	initial_buddy->is_free = 1;
	initial_buddy->base = 0;
	initial_buddy->size = MEMORY_SIZE;

	list_add(ALL_PARTITIONS, (void*) initial_buddy);
	list_add(FREE_PARTITIONS, (void*) initial_buddy);
}

void* buddy_alloc(int size)
{
	if (!is_power_of_two(size))
		size = upper_power_of_two(size);

	t_buddy* buddy = find_free_smaller_buddy_to_hold(size);

	while (buddy == NULL || buddy->size < size) {
		buddy = choose_victim_buddy();
		if (buddy != NULL) {
			buddy->is_free = 1;
			buddy->data = NULL;

			uint32_t* id_to_delete = malloc(sizeof(*id_to_delete));
			op_code* queue_deleted_msg = malloc(sizeof(*queue_deleted_msg));
			*id_to_delete = buddy->id_data;
			*queue_deleted_msg = buddy->queue;
			t_message_deleted* message_deleted = malloc(sizeof(*message_deleted));
			message_deleted->id = id_to_delete;
			message_deleted->queue = queue_deleted_msg;
			pthread_mutex_lock(&mutex_deleted_messages_ids);
			list_add(deleted_messages_ids, (void*) message_deleted);
			pthread_mutex_unlock(&mutex_deleted_messages_ids);

			log_deleted_partition(buddy->base);

			associate_buddies(buddy);
		}
	}

	if (buddy->size / 2 >= size) {
		create_buddies(buddy, size);
	}

	buddy->is_free = 0;
	list_remove(FREE_PARTITIONS, get_index_of_buddy_by_base(FREE_PARTITIONS, buddy->base));
	list_add(OCCUPIED_PARTITIONS, (void*) buddy);

	return (void*) buddy;
}

void create_buddies(t_buddy* buddy, int size)
{
	int buddy_size = buddy->size;
	int buddy_base = buddy->base;
	int buddy_index = get_index_of_buddy_by_base(ALL_PARTITIONS, buddy->base);
	while (buddy_size / 2 >= size) {
		buddy_size = buddy_size/2;
		buddy->size = buddy_size;

		t_buddy* new_buddy = malloc(sizeof(*new_buddy));
		new_buddy->id_data = 0;
		new_buddy->data = NULL;
		new_buddy->is_free = 1;
		new_buddy->size = buddy_size;
		new_buddy->base = buddy_base + buddy_size;

		list_add(FREE_PARTITIONS, (void*) new_buddy);
		list_add_in_index(ALL_PARTITIONS, buddy_index + 1, (void*) new_buddy);
	}
}

t_buddy* find_my_buddy(t_buddy* buddy)
{
	if (ALL_PARTITIONS == NULL)
		return NULL;

	t_link_element *element = ALL_PARTITIONS->head;
	t_buddy* other_buddy = (t_buddy*) (ALL_PARTITIONS->head->data);

	int index = 0;
	int buddy_base = buddy->base;
	int buddy_size = buddy->size;
	int my_buddy_base = buddy_base ^ buddy_size;
	while(element != NULL) {
		if (my_buddy_base == other_buddy->base)
			return other_buddy;

		element = element->next;
		other_buddy = element == NULL ? NULL : element->data;
		index++;
	}

	return NULL;
}

void associate_buddies(t_buddy* buddy)
{
	t_buddy* my_buddy = find_my_buddy(buddy);
	while (my_buddy != NULL && my_buddy->is_free && my_buddy->size == buddy->size && my_buddy->base != buddy->base) {

		int index = get_index_of_buddy_by_base(FREE_PARTITIONS, buddy->base);
		int my_buddy_index = get_index_of_buddy_by_base(FREE_PARTITIONS, my_buddy->base);
		if (buddy->base > my_buddy->base) {
			log_buddy_association(my_buddy->base, buddy->base);

			list_remove(FREE_PARTITIONS, index);
			list_replace(FREE_PARTITIONS, my_buddy_index, (void*) buddy);
			list_remove(ALL_PARTITIONS, get_index_of_buddy_by_base(ALL_PARTITIONS, buddy->base));
			list_replace(ALL_PARTITIONS, get_index_of_buddy_by_base(ALL_PARTITIONS, my_buddy->base), (void*) buddy);

			buddy->base = my_buddy->base;
		} else {
			log_buddy_association(buddy->base, my_buddy->base);
			list_remove(FREE_PARTITIONS, my_buddy_index);
			list_remove(ALL_PARTITIONS, get_index_of_buddy_by_base(ALL_PARTITIONS, my_buddy->base));
		}

		buddy->size = buddy->size * 2;

		t_buddy* bye_buddy = my_buddy;
		free(bye_buddy);
		my_buddy = find_my_buddy(buddy);
	}
}

t_buddy* find_free_smaller_buddy_to_hold(int size)
{
	int buddies_count = list_size(FREE_PARTITIONS);
	if (buddies_count == 0)
		return NULL;

	t_buddy* smaller_buddy = list_get(FREE_PARTITIONS, 0);
	for (int i=1; i < buddies_count; i++) {
		if (smaller_buddy->size == size)
			return smaller_buddy;

		t_buddy* candidate_buddy = list_get(FREE_PARTITIONS, i);
		if (smaller_buddy->size > candidate_buddy->size && candidate_buddy->size >= size)
			smaller_buddy = candidate_buddy;
	}
	return smaller_buddy->size >= size ? smaller_buddy : NULL;
}

t_buddy* choose_victim_buddy()
{
	t_buddy* victim = NULL;
	if (VICTIM_SELECTION_ALGORITHM == FIFO) {
		victim = fifo_find_victim_buddy();
	} else if (VICTIM_SELECTION_ALGORITHM == LRU) {
		victim = lru_find_victim_buddy();
		list_remove(OCCUPIED_PARTITIONS, get_index_of_buddy_by_base(OCCUPIED_PARTITIONS, victim->base));
	}

	if (victim != NULL)
		list_add(FREE_PARTITIONS, (void*) victim);

	return victim;
}

int get_index_of_buddy_by_base(t_list* buddies, uint32_t base_buddy)
{
	if (buddies->head == NULL)
		return -1;

	t_link_element *element = buddies->head;
	t_buddy* buddy = (t_buddy*) (buddies->head->data);

	int index = 0;
	while(element != NULL) {
		if (buddy->base == base_buddy)
			return index;

		element = element->next;
		buddy = element == NULL ? NULL : element->data;
		index++;
	}

	return -1;
}

t_buddy* fifo_find_victim_buddy()
{
	return (t_buddy*) get_first(OCCUPIED_PARTITIONS);
}

t_buddy* lru_find_victim_buddy()
{
	return (t_buddy*) get_first(lru_list);
}

void* find_data_buddy_by_id(uint32_t id)
{
	t_buddy* buddy = find_buddy_by_id(OCCUPIED_PARTITIONS, id);
	return buddy != NULL ? buddy->data : NULL;
}

t_buddy* find_buddy_by_id(t_list* list, uint32_t id)
{
	t_link_element* element = list->head;

	if (element == NULL)
		return NULL;

	t_buddy* buddy = (t_buddy*) (element->data);

	while(element != NULL && buddy->id_data != id) {
		element = element->next;
		buddy = element == NULL ? NULL : element->data;
	}

	return buddy;
}

unsigned upper_power_of_two(unsigned size)
{
    size -= 1;
    size |= (size >> 1);
    size |= (size >> 2);
    size |= (size >> 4);
    size |= (size >> 8);
    size |= (size >> 16);
    return size + 1;
}

int is_power_of_two(int number)
{
	return (number != 0) && ((number &(number - 1)) == 0);
}
