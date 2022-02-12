#ifndef CONEXION_H_
#define CONEXION_H_

	#include<stdio.h>
	#include<stdlib.h>
	#include<signal.h>
	#include<unistd.h>
	#include<sys/socket.h>
	#include<netdb.h>
	#include<string.h>

	#include "mensajes.h"
	#include<commons/collections/list.h>

	typedef struct
	{
		op_code codigo_operacion;
		uint32_t id;
		uint32_t id_correlativo;
		void* mensaje;
	} t_paquete;

	///////////////////////
	// ---- Cliente ---- //
	///////////////////////
	int crear_conexion(char *ip, char* puerto);

	//////////////////////
	// ---- Server ---- //
	//////////////////////
	int iniciar_servidor(char *ip, char* puerto);
	int esperar_cliente(int socket_servidor);

	////////////////////////
	// ---- Generales---- //
	////////////////////////
	void liberar_conexion(int un_socket);

	//////////////////////////////////
	// ---- Envíos de mensajes ---- //
	//////////////////////////////////
	/*
	 *  @NAME: enviar_mensaje
	 *  @RETURN: cantidad de bytes enviados o -1 en caso de falla
	 */
	int enviar_mensaje(op_code codigoOperacion, uint32_t id, uint32_t id_correlativo, void* mensaje, int un_socket);
	void* serializar_paquete(op_code codigoOperacion, uint32_t id, uint32_t id_correlativo, void* estructura, uint32_t* bytes);
	void serializar_variable(void* a_enviar, void* a_serializar, int tamanio, int *offset);
	void serializar_nombre(void* aEnviar, t_nombrePokemon nombrePokemon, int *offset);

	/////////////////////////////////////
	// ---- Recepción de mensajes ---- //
	/////////////////////////////////////
	/*
	 *  @NAME: recibir_variable
	 *  @RETURN: cantidad de bytes recibidos o -1 en caso de falla
	 */
	int recibir_variable(void* a_recibir, int tamanio, int socket_cliente);
	uint32_t obtener_cantidad_bytes_a_recibir(int socket_cliente);
	/*
	 *  @NAME: recibir_paquete
	 *  @RETURN: paquete recibido o NULL en caso de falla en el recv
	 */
	t_paquete* recibir_paquete(int socket_cliente, char** nombre_recibido, uint32_t* tamanio_recibido);
	void deserializar_paquete(void* stream, t_paquete* paquete_recibido, int* offset, uint32_t bytes, char** nombre_recibido);
	void copiar_nombre(t_nombrePokemon* estructuraNombre, void* stream, int* offset);
	void copiar_coordenadas(t_coordenadas* estructuraCoordenadas, void* stream, int* offset);
	void copiar_variable(void* variable, void* stream, int* offset, int size);
	void free_paquete_recibido(char* nombre_recibido, t_paquete* paquete_recibido);

	//////////////////////////////////////
	// ---- Respuestas de mensajes ---- //
	//////////////////////////////////////
	void enviar_id_respuesta(uint32_t id_msg, int socket_cliente);
	uint32_t recibir_id(int socket_cliente);
	int informar_ack(int socket_server);

	/////////////////////////////
	// ---- Suscripciones ---- //
	/////////////////////////////
	/*
	 *  @NAME: suscribirse_a_cola
	 *  @RETURN: cantidad de bytes enviados o -1 en caso de falla
	 */
	int suscribirse_a_cola(t_suscripcion_msg* estructuraSuscripcion, int socket_servidor);
	int desuscribirse_de_cola(int socket_servidor);
	t_list* respuesta_suscripcion_obtener_paquetes(int socket_servidor, uint32_t* cant_paquetes_recibidos);

#endif /* CONEXION_H_ */
