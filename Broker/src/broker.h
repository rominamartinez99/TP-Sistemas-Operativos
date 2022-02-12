/*
 ============================================================================
 Name        : Broker
 Author      : Fran and Co
 Description : Header Proceso Broker
 ============================================================================
 */

#ifndef BROKER_H_
#define BROKER_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <pthread.h>

#include "nuestras-commons/conexion.h"
#include "nuestras-commons/mensajes.h"
#include "messages_queues.h"
#include "memory.h"

#define BROKER_NAME "broker"
#define BROKER_CONFIG "broker.config"

uint32_t ID_COUNTER;
pthread_t thread;

// Colas de mensajes y listas de suscriptores
t_queue* NEW_POKEMON_QUEUE;
t_queue* APPEARED_POKEMON_QUEUE;
t_queue* CATCH_POKEMON_QUEUE;
t_queue* CAUGHT_POKEMON_QUEUE;
t_queue* GET_POKEMON_QUEUE;
t_queue* LOCALIZED_POKEMON_QUEUE;
t_list* NEW_POKEMON_SUBSCRIBERS;
t_list* APPEARED_POKEMON_SUBSCRIBERS;
t_list* CATCH_POKEMON_SUBSCRIBERS;
t_list* CAUGHT_POKEMON_SUBSCRIBERS;
t_list* GET_POKEMON_SUBSCRIBERS;
t_list* LOCALIZED_POKEMON_SUBSCRIBERS;


// Mutex
pthread_mutex_t mutex_id_counter = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_new_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_appeared_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_get_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_localized_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_catch_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_caught_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_new_susc = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_appeared_susc = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_get_susc = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_localized_susc = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_catch_susc = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_caught_susc = PTHREAD_MUTEX_INITIALIZER;

// Arrays
t_queue* COLAS_MENSAJES[7];
t_list* SUSCRIPTORES_MENSAJES[7];
pthread_mutex_t MUTEX_COLAS[7];
pthread_mutex_t MUTEX_SUSCRIPTORES[7];

// Config
t_config* CONFIG;

int init_server();
void init_message_queues();
void init_suscriber_lists();
void init_logger();
void init_config();
void init_memory();

int esperar_cliente(int socket_server);
void serve_client(int* socket_client);
void process_new_message(op_code cod_op, uint32_t id_correlative, void* received_message, uint32_t size_message, int socket_client);
void process_suscription(t_suscripcion_msg* estructuraSuscripcion, int socket_suscriptor);
t_list* inform_subscribers(op_code codigo, void* mensaje, uint32_t id, uint32_t id_correlative, t_list* suscriptores, pthread_mutex_t mutex);
void reply_to_new_subscriber(op_code code, t_queue* message_queue, t_subscriber* subscriber, uint32_t* messages_count, t_list* enqueue_messages);
void send_enqueued_messages(uint32_t cantidad_mensajes, uint32_t tamanio_stream, t_list* paquetes_serializados, t_list* tamanio_paquetes,
		t_list* mensajes_encolados, t_subscriber* subscriber);
void remove_subscriber_if_temporal(t_list* subscribers, t_subscriber* subscriber, uint32_t tiempo, pthread_mutex_t mutex);
void receive_ack(t_list* mensajes_encolados, uint32_t cantidad_mensajes, t_subscriber* subscriber, uint32_t id, op_code code);
void receive_multiples_ack(op_code codigo, uint32_t id, t_list* suscriptores_informados, pthread_mutex_t mutex);
void remove_messages_by_id(t_list* ids_messages_deleted, int ids_count);

uint32_t generate_id();

void terminar_programa(int socket, t_log* logger);

#endif /* BROKER_H_ */
