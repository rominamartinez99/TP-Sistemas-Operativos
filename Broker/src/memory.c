/*
 ============================================================================
 Name        : Broker
 Author      : Fran and Co
 Description : Memory
 ============================================================================
 */

#include "memory.h"

pthread_mutex_t mutex_memory = PTHREAD_MUTEX_INITIALIZER;

void load_memory(int size, int min_partition_size, int frequency, t_memory_algorithm memory_alg, t_selection_algorithm victim_alg, t_selection_algorithm partition_alg, char* dump_path)
{
	MEMORY = malloc(size);
	MEMORY_SIZE = size;
	MIN_PARTITION_SIZE = min_partition_size;
	COMPACTION_FREQUENCY = frequency;
	MEMORY_ALGORITHM = memory_alg;
	VICTIM_SELECTION_ALGORITHM = victim_alg;
	PARTITION_SELECTION_ALGORITHM = partition_alg;
	DUMP_PATH = dump_path;

	lru_list = list_create();
	deleted_messages_ids = list_create();
	FREE_PARTITIONS = list_create();
	OCCUPIED_PARTITIONS = list_create();
	ALL_PARTITIONS = list_create();

	if (MEMORY_ALGORITHM == BUDDY_SYSTEM) {
		buddy_init();
	} else if (MEMORY_ALGORITHM == DYNAMIC_PARTITIONS) {
		dp_init();
	}
}

void* memory_alloc(int size)
{
	if (size > MEMORY_SIZE)
		return NULL;

	void* allocated = NULL;
	pthread_mutex_lock(&mutex_memory);
	if (MEMORY_ALGORITHM == BUDDY_SYSTEM) {
		allocated = buddy_alloc(size);
	} else if (MEMORY_ALGORITHM == DYNAMIC_PARTITIONS) {
		allocated = dp_alloc(size);
	}
	pthread_mutex_unlock(&mutex_memory);

	return allocated;
}

void* memory_copy(t_copy_args* args)
{
	void* data = NULL;
	if (MEMORY_ALGORITHM == BUDDY_SYSTEM) {
		t_buddy* buddy = args->alloc;
		buddy->id_data = args->id;
		buddy->queue = args->queue;

		data = memcpy(MEMORY + buddy->base, args->data, args->data_size);
		buddy->data = data;

		log_new_message_in_memory(buddy->id_data, buddy->base);
	} else if (MEMORY_ALGORITHM == DYNAMIC_PARTITIONS) {
		t_partition* partition = args->alloc;
		partition->id_data = args->id;
		partition->queue = args->queue;

		data = memcpy(MEMORY + partition->base, args->data, args->data_size); //funciona bien pero valgrind dice invalid read of size X
		partition->data = data;

		log_new_message_in_memory(partition->id_data, partition->base);
	}

	return data;
}

void* memory_get(uint32_t id)
{
	void* data = NULL;
	pthread_mutex_lock(&mutex_memory);
	if (MEMORY_ALGORITHM == BUDDY_SYSTEM) {
		data = find_data_buddy_by_id(id);
	} else if (MEMORY_ALGORITHM == DYNAMIC_PARTITIONS) {
		data = find_data_partition_by_id(id);
	}
	pthread_mutex_unlock(&mutex_memory);
	return data;
}

uint32_t get_lru_partition_id(int index) {
	if (MEMORY_ALGORITHM == BUDDY_SYSTEM) {
		t_buddy* buddy = list_get(lru_list, index);
		return buddy != NULL ? buddy->id_data : -1;
	} else if (MEMORY_ALGORITHM == DYNAMIC_PARTITIONS) {
		t_partition* partition = list_get(lru_list, index);
		return partition != NULL ? partition->id_data : -1;
	}
	return -1;
}

int get_index_of_lru_partition(uint32_t id_data)
{
	int lru_size = list_size(lru_list);

	for (int i=0; i < lru_size; i++) {
		uint32_t id_lru = get_lru_partition_id(i);
		if (id_lru == id_data)
			return i;
	}

	return -1;
}

void write_dump_time_info(FILE* dump_file)
{
  time_t now = time(NULL);
  struct tm* timeinfo;
  setenv("TZ", "America/Buenos_Aires", 1);
  timeinfo = localtime(&now);
  fprintf(dump_file, "%d/%d/%d %d:%d:%d\n", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

void write_partitions_info(FILE* dump_file)
{
	int partition_number = 1;
	int base, limit, size, lru, id, is_free;
	char* queue;

	int partitions_size = list_size(ALL_PARTITIONS);

	pthread_mutex_lock(&mutex_lru_list);
	for (int i=0; i < partitions_size; i++) {
		if (MEMORY_ALGORITHM == BUDDY_SYSTEM) {
			t_buddy* buddy = (t_buddy*) list_get(ALL_PARTITIONS, i);
			base = buddy->base;
			size = buddy->size;
			limit = base + size - 1;
			queue = op_code_a_string(buddy->queue);
			id = buddy->id_data;
			is_free = buddy->is_free;
		} else if (MEMORY_ALGORITHM == DYNAMIC_PARTITIONS) {
			t_partition* partition = (t_partition*) list_get(ALL_PARTITIONS, i);
			base = partition->base;
			size = partition->size;
			limit = base + size - 1;
			queue = op_code_a_string(partition->queue);
			id = partition->id_data;
			is_free = partition->is_free;
		}
		lru = VICTIM_SELECTION_ALGORITHM == LRU ? get_index_of_lru_partition(id) : 0;

		if (is_free)
			fprintf(dump_file,"Partición %d: %d - %d.    [L]    Size: %db\n", partition_number, base, limit, size);
		else
			fprintf(dump_file,"Partición %d: %d - %d.    [X]    Size: %db    LRU:<%d>    Cola:<%s>    ID:<%d>\n", partition_number, base, limit, size, lru, queue, id);

		partition_number++;
	}
	pthread_mutex_unlock(&mutex_lru_list);

}

void memory_dump()
{
	pthread_mutex_lock(&mutex_memory);
	FILE* dump_file = fopen(DUMP_PATH, "w");

	const char* line = "-----------------------------------------------------------------------------------------------------------------------------";

	fprintf(dump_file,"%s\n", line);
	write_dump_time_info(dump_file);
	write_partitions_info(dump_file);
	fprintf(dump_file,"%s\n", line);

	log_dump();
	fclose(dump_file);
	pthread_mutex_unlock(&mutex_memory);
}

void ids_message_destroyer(void* message)
{
	t_message_deleted* message_deleted = (t_message_deleted*) message;
	free(message_deleted->id);
	free(message_deleted->queue);
	free(message_deleted);
}

t_list* get_victim_messages_ids(int* element_count)
{
	*element_count = list_size(deleted_messages_ids);
	return deleted_messages_ids;
}

void notify_all_victim_messages_deleted()
{
	list_clean_and_destroy_elements(deleted_messages_ids, ids_message_destroyer);
}

void notify_message_used(uint32_t id_message)
{
	if (VICTIM_SELECTION_ALGORITHM == LRU) {
		pthread_mutex_lock(&mutex_lru_list);

		void* partition = NULL;
		int index = get_index_of_lru_partition(id_message);
		if (index >= 0) {
			partition = list_remove(lru_list, index);
		} else {
			if (MEMORY_ALGORITHM == BUDDY_SYSTEM) {
				partition = (void*) find_buddy_by_id(OCCUPIED_PARTITIONS, id_message);
			} else if (MEMORY_ALGORITHM == DYNAMIC_PARTITIONS) {
				partition = (void*) find_partition_by_id(OCCUPIED_PARTITIONS, id_message);
			}
		}

		list_add(lru_list, partition);

		pthread_mutex_unlock(&mutex_lru_list);
	}
}

void end_memory()
{

}
