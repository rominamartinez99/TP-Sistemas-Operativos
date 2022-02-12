/*
 ============================================================================
 Name        : Broker
 Author      : Fran and Co
 Description : Dynamic Partitions Algorithm
 ============================================================================
 */

#include "dynamic_partitions.h"

void dp_init()
{
	SEARCH_FAILURE_COUNTER = 0;

	t_partition* initial_partition = malloc(sizeof(*initial_partition));
	initial_partition->id_data = 0;
	initial_partition->data = NULL;
	initial_partition->is_free = 1;
	initial_partition->base = 0;
	initial_partition->size = MEMORY_SIZE;

	list_add(FREE_PARTITIONS, (void*) initial_partition);
	list_add(ALL_PARTITIONS, (void*) initial_partition);
}

void* dp_alloc(int size)
{
	t_partition* partition = find_free_partition(size);

	int index_of_victim = -1;
	while (partition == NULL || partition->size < size) {
		index_of_victim = -1;
		partition = choose_victim_partition();
		if (partition != NULL) {
			partition->is_free = 1;
			partition->data = NULL;

			uint32_t* id_to_delete = malloc(sizeof(*id_to_delete));
			op_code* queue_deleted_msg = malloc(sizeof(*queue_deleted_msg));
			*id_to_delete = partition->id_data;
			*queue_deleted_msg = partition->queue;
			t_message_deleted* message_deleted = malloc(sizeof(*message_deleted));
			message_deleted->id = id_to_delete;
			message_deleted->queue = queue_deleted_msg;
			pthread_mutex_lock(&mutex_deleted_messages_ids);
			list_add(deleted_messages_ids, (void*) message_deleted);
			pthread_mutex_unlock(&mutex_deleted_messages_ids);

			log_deleted_partition(partition->base);

			index_of_victim = list_add(FREE_PARTITIONS, (void*) partition);

			consolidate_free_partitions(partition, &index_of_victim);
		}

		if (compact_memory() == 1) {
			partition = list_get(FREE_PARTITIONS, 0);
			index_of_victim = 0;
		}
	}

	if (partition->size > size && partition->size - size >= MIN_PARTITION_SIZE) {
		adjust_partition_size(partition, size);
	}

	partition->is_free = 0;
	list_add(OCCUPIED_PARTITIONS, (void*) partition);
	if (index_of_victim >= 0)
		list_remove(FREE_PARTITIONS, index_of_victim);

	SEARCH_FAILURE_COUNTER = 0;

	return (void*) partition;
}

void consolidate_free_partitions(t_partition* new_free_partition, int* index_free)
{
	int index_all = get_index_of_partition_by_base(ALL_PARTITIONS, new_free_partition->base);

	int previous_index = index_all - 1;
	t_partition* previous_partition = list_get(ALL_PARTITIONS, previous_index);
	while (previous_partition != NULL && previous_partition->is_free) {

		new_free_partition->size += previous_partition->size;
		new_free_partition->base = previous_partition->base;

		list_remove(FREE_PARTITIONS, *index_free);
		list_replace(FREE_PARTITIONS, get_index_of_partition_by_base(FREE_PARTITIONS, previous_partition->base), (void*) new_free_partition);
		list_remove(ALL_PARTITIONS, index_all);
		list_replace(ALL_PARTITIONS, previous_index, (void*) new_free_partition);

		free(previous_partition);
		index_all = previous_index;
		previous_index--;
		previous_partition = list_get(ALL_PARTITIONS, previous_index);
	}

	int next_index = index_all + 1;
	t_partition* next_partition = list_get(ALL_PARTITIONS, next_index);
	while (next_partition != NULL && next_partition->is_free) {

		new_free_partition->size += next_partition->size;

		list_remove(FREE_PARTITIONS, get_index_of_partition_by_base(FREE_PARTITIONS, next_partition->base));
		list_remove(ALL_PARTITIONS, next_index);

		free(next_partition);
		index_all = next_index;
		next_index++;
		next_partition = list_get(ALL_PARTITIONS, next_index);
	}

	*index_free = get_index_of_partition_by_base(FREE_PARTITIONS, new_free_partition->base);
}

void adjust_partition_size(t_partition* partition, int size)
{
	t_partition* new_partition = malloc(sizeof(*new_partition));
	new_partition->data = NULL;
	new_partition->id_data = 0;
	new_partition->is_free = 1;

	if (size <= MIN_PARTITION_SIZE) {
		new_partition->size = partition->size - MIN_PARTITION_SIZE;
		new_partition->base = partition->base + MIN_PARTITION_SIZE;
		partition->size = MIN_PARTITION_SIZE;
	} else {
		new_partition->size = partition->size - size;
		new_partition->base = partition->base + size;
		partition->size = size;
	}

	list_add(FREE_PARTITIONS, (void*) new_partition);
	int index_of_partition = get_index_of_partition_by_base(ALL_PARTITIONS, partition->base);
	list_add_in_index(ALL_PARTITIONS, index_of_partition + 1, (void*) new_partition);
}

t_partition* find_free_partition(int size)
{
	if (PARTITION_SELECTION_ALGORITHM == FIRST_FIT) {
		return first_fit_find_free_partition(size);
	} else if (PARTITION_SELECTION_ALGORITHM == BEST_FIT) {
		return best_fit_find_free_partition(size);
	}
	return NULL;
}

t_partition* choose_victim_partition()
{
	if (VICTIM_SELECTION_ALGORITHM == FIFO) {
		return fifo_find_victim_partition();
	} else if (VICTIM_SELECTION_ALGORITHM == LRU) {
		return lru_find_victim_partition();
	}
	return NULL;
}

void compact_occupied_list(int* previous_occupied_base, int* previous_occupied_size)
{
	int occupied_list_size = list_size(OCCUPIED_PARTITIONS);
	if (occupied_list_size > 0) {

		t_partition* occupied_partition = list_get(OCCUPIED_PARTITIONS, 0);
		occupied_partition->base = 0;

		int previous_base = occupied_partition->base;
		int previous_size = occupied_partition->size;

		int index_occupied = 1;
		while(index_occupied < occupied_list_size) {
			occupied_partition = list_get(OCCUPIED_PARTITIONS, index_occupied);
			occupied_partition->base = previous_base + previous_size;
			previous_base = occupied_partition->base;
			previous_size = occupied_partition->size;
			index_occupied++;
		}
		*previous_occupied_base = previous_base;
		*previous_occupied_size = previous_size;
	}
}

t_partition* compact_free_list()
{
	int size_compacted_partition = 0;
	t_partition* compacted_partition = malloc(sizeof(*compacted_partition));
	compacted_partition->data = NULL;
	compacted_partition->id_data = 0;
	compacted_partition->is_free = 1;

	int free_list_size = list_size(FREE_PARTITIONS);
	for (int i=0; i < free_list_size; i++) {
		t_partition* free_partition = list_get(FREE_PARTITIONS, i);
		size_compacted_partition += free_partition->size;
		compacted_partition->queue = free_partition->queue;
		int index_all = get_index_of_partition_by_base(ALL_PARTITIONS, free_partition->base);
		list_remove(ALL_PARTITIONS, index_all);
		free(free_partition);
	}

	list_clean(FREE_PARTITIONS);

	compacted_partition->size = size_compacted_partition;
	list_add(FREE_PARTITIONS, (void*) compacted_partition);
	return compacted_partition;
}

void sort_all_partitions_by_base()
{
	bool sort_by_base(void* partition1, void* partition2) {
		t_partition* partition = (t_partition*) partition1;
		t_partition* other_partition = (t_partition*) partition2;
		return partition->base < other_partition->base;
	}

	list_sort(ALL_PARTITIONS, sort_by_base);
}

void sort_memory_by_base()
{
	int backup_size = 0;
	void get_backup_size(void* element) {
		t_partition* partition = (t_partition*) element;
		backup_size += partition->size;
	}
	list_iterate(OCCUPIED_PARTITIONS, get_backup_size);

	void* copy_target;
	void memcpy_to_target(void* element) {
		t_partition* partition = (t_partition*) element;
		void* data = memcpy(copy_target + partition->base, partition->data, partition->size);
		partition->data = data;
	}

	void* backup_memory = malloc(backup_size);
	copy_target = backup_memory;
	list_iterate(OCCUPIED_PARTITIONS, memcpy_to_target);

	copy_target = MEMORY;
	list_iterate(OCCUPIED_PARTITIONS, memcpy_to_target);

	free(backup_memory);
}

int compact_memory()
{
	SEARCH_FAILURE_COUNTER++;

	int occ_size = list_size(OCCUPIED_PARTITIONS);
	if (COMPACTION_FREQUENCY == -1 && occ_size == 0)
		SEARCH_FAILURE_COUNTER = COMPACTION_FREQUENCY;

	if (SEARCH_FAILURE_COUNTER == COMPACTION_FREQUENCY) {
		t_partition* compacted_free_partition = compact_free_list();
		int previous_base = 0;
		int previous_size = 0;
		compact_occupied_list(&previous_base, &previous_size);
		compacted_free_partition->base = previous_base + previous_size;
		list_add(ALL_PARTITIONS, compacted_free_partition);

		sort_all_partitions_by_base();
		sort_memory_by_base();

		log_compactation();
		SEARCH_FAILURE_COUNTER = 0;
		return 1;
	}
	return 0;
}

int get_index_of_partition_by_base(t_list* partitions, uint32_t base_partition)
{
	if (partitions->head == NULL)
		return -1;

	t_link_element *element = partitions->head;
	t_partition* partition = (t_partition*) (partitions->head->data);

	int index = 0;
	while(element != NULL) {
		if (partition->base == base_partition)
			return index;

		element = element->next;
		partition = element == NULL ? NULL : element->data;
		index++;
	}

	return -1;
}

void* find_data_partition_by_id(uint32_t id)
{
	t_partition* partition = find_partition_by_id(OCCUPIED_PARTITIONS, id);
	return partition != NULL ? partition->data : NULL;
}

t_partition* find_partition_by_id(t_list* list, uint32_t id)
{
	t_link_element* element = list->head;

	if (element == NULL)
		return NULL;

	t_partition* partition = (t_partition*) (element->data);

	while(element != NULL && partition->id_data != id) {
		element = element->next;
		partition = element == NULL ? NULL : element->data;
	}

	return partition;
}

t_partition* first_fit_find_free_partition(int size)
{
	t_link_element* element = FREE_PARTITIONS->head;

	if (element == NULL)
		return NULL;

	t_partition* partition = (t_partition*) element->data;

	int index = 0;
	while (element != NULL) {
		if (partition->size >= size)
			return (t_partition*) list_remove(FREE_PARTITIONS, index);

		element = element->next;
		partition = element == NULL ? NULL : element->data;
		index++;
	}

	return NULL;
}

t_partition* best_fit_find_free_partition(int size)
{
	t_link_element* element = FREE_PARTITIONS->head;

	if (element == NULL)
		return NULL;

	t_partition* partition = (t_partition*) element->data;
	t_partition* best_choise = partition;

	int index = 0;
	int best_index = 0;
	while (element != NULL) {
		if (partition->size == size)
			return (t_partition*) list_remove(FREE_PARTITIONS, index);

		if (partition->size > size && partition->size < best_choise->size) {
			best_index = index;
			best_choise = partition;
		}

		element = element->next;
		partition = element == NULL ? NULL : element->data;
		index++;
	}

	if (best_choise->size < size)
		return NULL;

	return (t_partition*) list_remove(FREE_PARTITIONS, best_index);
}

t_partition* fifo_find_victim_partition()
{
	return (t_partition*) get_first(OCCUPIED_PARTITIONS);
}

t_partition* lru_find_victim_partition()
{
	t_partition* partition = (t_partition*) get_first(lru_list);
	list_remove(OCCUPIED_PARTITIONS, get_index_of_partition_by_base(OCCUPIED_PARTITIONS, partition->base));
	return partition;
}
