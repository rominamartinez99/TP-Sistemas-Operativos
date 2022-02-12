/*
 ============================================================================
 Name        : Messages Queues
 Author      : Fran and Co
 Description : Funciones de creación de colas que enrealidad no son colas
 ============================================================================
 */

#include "messages_queues.h"

/////////////////////////////////
// ---- Colas de mensajes ---- //
/////////////////////////////////

t_queue* create_message_queue()
{
	return queue_create();
}

t_enqueued_message* push_message_queue(t_queue* queue, uint32_t ID, uint32_t ID_correlativo, pthread_mutex_t mutex)
{
	t_enqueued_message* data = malloc(sizeof(*data));
	data->subscribers_ack = list_create();
	data->subscribers_informed = list_create();
	data->ID = ID;
	data->ID_correlativo = ID_correlativo;

	queue_push(queue, (void*) data);

	return data;
}

t_enqueued_message* pop_message_queue(t_queue* queue, pthread_mutex_t mutex)
{
	pthread_mutex_lock(&mutex);
	t_enqueued_message* message = (t_enqueued_message*) queue_pop(queue);
	pthread_mutex_unlock(&mutex);
	return message;
}

t_enqueued_message* get_message_by_index(t_queue* queue, int index)
{
	t_enqueued_message* message = (t_enqueued_message*) list_get(queue->elements, index);
	return message;
}

void inform_message_sent_to(t_enqueued_message* message, t_subscriber* subscriber)
{
	list_add(message->subscribers_informed, (void*)subscriber);
}

void inform_message_ack_from(t_enqueued_message* message, t_subscriber* subscriber)
{
	list_add(message->subscribers_ack, (void*)subscriber);
}

int is_same_id(uint32_t data_id, uint32_t id)
{
	return data_id == id;
}

t_enqueued_message* find_message_by_id(t_queue* queue, uint32_t id)
{
	t_link_element* element = queue->elements->head;

	if (element == NULL)
		return NULL;

	t_enqueued_message* message = (t_enqueued_message*) (queue->elements->head->data);

	while(element != NULL && !is_same_id(message->ID, id)) {
		element = element->next;
		message = element == NULL ? NULL : element->data;
	}

	return message;
}

t_enqueued_message* find_message_by_id_correlative(t_queue* queue, uint32_t id)
{
	t_link_element *element = queue->elements->head;

	if (element == NULL)
		return NULL;

	t_enqueued_message* message = (t_enqueued_message*) (queue->elements->head->data);

	while(element != NULL && !is_same_id(message->ID_correlativo, id)) {
		element = element->next;
		message = element == NULL ? NULL : element->data;
	}

	return message;
}

void remove_message_by_id(t_queue* queue, uint32_t id)
{
	t_link_element *element = queue->elements->head;
	t_enqueued_message* message = (t_enqueued_message*) (queue->elements->head->data);
	int position = 0;

	while(element != NULL && !is_same_id(message->ID, id)) {
		element = element->next;
		message = element == NULL ? NULL : element->data;
		position++;
	}

	message = (t_enqueued_message*) list_remove(queue->elements, position);
	element_destroyer_mq((void*) message);

}

void remove_message_by_id_correlativo(t_queue* queue, uint32_t id)
{
	t_link_element *element = queue->elements->head;
	t_enqueued_message* message = (t_enqueued_message*) (queue->elements->head->data);
	int position = 0;

	while(element != NULL && !is_same_id(message->ID_correlativo, id)) {
		element = element->next;
		message = element == NULL ? NULL : element->data;
		position++;
	}

	if (message != NULL) {
		message = (t_enqueued_message*) list_remove(queue->elements, position);
		element_destroyer_mq((void*) message);
	}
}

void element_destroyer_mq(void* message)
{
	t_enqueued_message* message_enqueue = (t_enqueued_message*) message;
	free_subscribers_list(message_enqueue->subscribers_ack);
	free_subscribers_list(message_enqueue->subscribers_informed);
	free(message_enqueue);
}

int size_message_queue(t_queue* queue)
{
	return queue_size(queue);
}

int is_empty_message_queue(t_queue* queue)
{
	return queue_is_empty(queue);
}

void free_message_queue(t_queue* queue)
{
	queue_destroy_and_destroy_elements(queue, element_destroyer_mq);
}


/////////////////////////////////////
// ---- Suscriptores de colas ---- //
/////////////////////////////////////

void subscribe_process(t_list* subscribers, t_subscriber* subscriber, pthread_mutex_t mutex)
{
	pthread_mutex_lock(&mutex);
	list_add(subscribers, (void*) subscriber);
	pthread_mutex_unlock(&mutex);
}

void unsubscribe_process(t_list* subscribers, t_subscriber* subscriber, pthread_mutex_t mutex)
{
	pthread_mutex_lock(&mutex);

	int index = get_index_of_subscriber(subscribers, subscriber->id_subscriber);
	if (index != -1)
		list_remove(subscribers, index);

	pthread_mutex_unlock(&mutex);
}

int get_index_of_subscriber(t_list* subscribers, uint32_t id_subscriber)
{
	if (subscribers->head == NULL)
		return -1;

	t_link_element *element = subscribers->head;
	t_subscriber* subscriber_listed = (t_subscriber*) (subscribers->head->data);

	int index = 0;
	while(element != NULL) {
		if (is_same_id(subscriber_listed->id_subscriber, id_subscriber))
			return index;

		element = element->next;
		subscriber_listed = element == NULL ? NULL : element->data;
		index++;
	}

	return -1;
}

t_subscriber* get_subscriber_by_id(t_list* subscribers, uint32_t id_subscriber)
{
	int index = get_index_of_subscriber(subscribers, id_subscriber);

	return index >= 0 ? (t_subscriber*) list_get(subscribers, index) : NULL;
}

int isSubscriberListed(t_list* subscribers, uint32_t id_subscriber)
{
	return !list_is_empty(subscribers) && get_index_of_subscriber(subscribers, id_subscriber) >= 0;
}

void add_new_informed_subscriber_to_mq(t_list* messages_in_queue, uint32_t number_of_messages, t_subscriber* subscriber) {
	for (int i=0; i < number_of_messages; i++) {
		t_enqueued_message* message = (t_enqueued_message*) list_get(messages_in_queue, i);
		if (!isSubscriberListed(message->subscribers_informed, subscriber->id_subscriber)) {
			list_add(message->subscribers_informed, subscriber);
			log_message_to_subscriber(subscriber->id_subscriber, message->ID);
		}
	}
}

void add_new_ack_suscriber_to_mq(t_list* messages_in_queue, uint32_t number_of_messages, t_subscriber* subscriber) {

	for (int i=0; i < number_of_messages; i++) {
		t_enqueued_message* message = (t_enqueued_message*) list_get(messages_in_queue, i);
		if (!isSubscriberListed(message->subscribers_ack, subscriber->id_subscriber)) {
			list_add(message->subscribers_ack, subscriber);
			log_ack_from_subscriber(subscriber->id_subscriber, message->ID);
		}
	}
}


void free_subscribers_list(t_list* subscribers)
{
	list_destroy(subscribers);
}


