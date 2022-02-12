#include "conexion.h"


///////////////////////
// ---- Cliente ---- //
///////////////////////

int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
		socket_cliente = -1;

	freeaddrinfo(server_info);

	return socket_cliente;
}


//////////////////////
// ---- Server ---- //
//////////////////////

int iniciar_servidor(char *ip, char* puerto)
{
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

	listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;

	socklen_t tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

	return socket_cliente;
}

//////////////////////////////////
// ---- Envíos de mensajes ---- //
//////////////////////////////////

int enviar_mensaje(op_code codigoOperacion, uint32_t id, uint32_t id_correlativo, void* mensaje, int socket_envio)
{
	uint32_t bytes;
	void* paqueteAEnviar = serializar_paquete(codigoOperacion, id, id_correlativo, mensaje, &bytes);
	fflush(stdout);
	int status = send(socket_envio, paqueteAEnviar, bytes + sizeof(bytes), MSG_NOSIGNAL);
	free(paqueteAEnviar);
	return status;
}

void* serializar_paquete(op_code codigo_operacion, uint32_t id, uint32_t id_correlativo, void* estructura, uint32_t* bytes)
{
	int offset = 0;
	uint32_t codigo_a_serializar = -1;
	*bytes = sizeof(codigo_a_serializar) + sizeof(id) + sizeof(id_correlativo);
	void* a_enviar;

	switch(codigo_operacion)
	{
		case SUSCRIPCION: ;
			codigo_a_serializar = 7;
			t_suscripcion_msg* estSuscripcion = estructura;
			*bytes += sizeof(estSuscripcion->id_proceso)
					+ sizeof(estSuscripcion->tipo_cola)
					+ sizeof(estSuscripcion->temporal);
			a_enviar = malloc(*bytes + sizeof(*bytes));

			serializar_variable(a_enviar, bytes, sizeof(uint32_t), &offset);
			serializar_variable(a_enviar, &codigo_a_serializar, sizeof(codigo_a_serializar), &offset);
			serializar_variable(a_enviar, &id, sizeof(id), &offset);
			serializar_variable(a_enviar, &id_correlativo, sizeof(id_correlativo), &offset);
			serializar_variable(a_enviar, &(estSuscripcion->id_proceso), sizeof(estSuscripcion->id_proceso), &offset);
			serializar_variable(a_enviar, &(estSuscripcion->tipo_cola), sizeof(estSuscripcion->tipo_cola), &offset);
			serializar_variable(a_enviar, &(estSuscripcion->temporal), sizeof(estSuscripcion->temporal), &offset);
			break;
		case NEW_POKEMON: ;
			codigo_a_serializar = 1;
			t_newPokemon_msg* estNew = estructura;
			*bytes += estNew->nombre_pokemon.nombre_lenght
					+ sizeof(estNew->nombre_pokemon.nombre_lenght)
					+ sizeof(estNew->cantidad_pokemons)
					+ sizeof(estNew->coordenadas.posX)
					+ sizeof(estNew->coordenadas.posY);
			a_enviar = malloc(*bytes + sizeof(*bytes));

			serializar_variable(a_enviar, bytes, sizeof(uint32_t), &offset);
			serializar_variable(a_enviar, &codigo_a_serializar, sizeof(codigo_a_serializar), &offset);
			serializar_variable(a_enviar, &id, sizeof(id), &offset);
			serializar_variable(a_enviar, &id_correlativo, sizeof(id_correlativo), &offset);
			serializar_nombre(a_enviar, estNew->nombre_pokemon, &offset);
			serializar_variable(a_enviar, &(estNew->coordenadas.posX), sizeof(estNew->coordenadas.posX), &offset);
			serializar_variable(a_enviar, &(estNew->coordenadas.posY), sizeof(estNew->coordenadas.posY), &offset);
			serializar_variable(a_enviar, &(estNew->cantidad_pokemons), sizeof(estNew->cantidad_pokemons), &offset);

			break;
		case APPEARED_POKEMON: ;
			codigo_a_serializar = 2;
			t_appearedPokemon_msg* estAppeared = estructura;
			*bytes += estAppeared->nombre_pokemon.nombre_lenght
					+ sizeof(estAppeared->nombre_pokemon.nombre_lenght)
					+ sizeof(estAppeared->coordenadas.posX)
					+ sizeof(estAppeared->coordenadas.posY);
			a_enviar = malloc(*bytes + sizeof(*bytes));

			serializar_variable(a_enviar, bytes, sizeof(uint32_t), &offset);
			serializar_variable(a_enviar, &codigo_a_serializar, sizeof(codigo_a_serializar), &offset);
			serializar_variable(a_enviar, &id, sizeof(id), &offset);
			serializar_variable(a_enviar, &id_correlativo, sizeof(id_correlativo), &offset);
			serializar_nombre(a_enviar, estAppeared->nombre_pokemon, &offset);
			serializar_variable(a_enviar, &(estAppeared->coordenadas.posX), sizeof(estAppeared->coordenadas.posX), &offset);
			serializar_variable(a_enviar, &(estAppeared->coordenadas.posY), sizeof(estAppeared->coordenadas.posY), &offset);
			break;
		case GET_POKEMON: ;
			codigo_a_serializar = 5;
			t_getPokemon_msg* estGet = estructura;
			*bytes += estGet->nombre_pokemon.nombre_lenght
					+ sizeof(estGet->nombre_pokemon.nombre_lenght);
			a_enviar = malloc(*bytes + sizeof(*bytes));

			serializar_variable(a_enviar, bytes, sizeof(uint32_t), &offset);
			serializar_variable(a_enviar, &codigo_a_serializar, sizeof(codigo_a_serializar), &offset);
			serializar_variable(a_enviar, &id, sizeof(id), &offset);
			serializar_variable(a_enviar, &id_correlativo, sizeof(id_correlativo), &offset);
			serializar_nombre(a_enviar, estGet->nombre_pokemon, &offset);
			break;
		case LOCALIZED_POKEMON: ;
			codigo_a_serializar = 6;
			t_localizedPokemon_msg* estLocalized = estructura;
			*bytes += estLocalized->nombre_pokemon.nombre_lenght
					+ sizeof(estLocalized->nombre_pokemon.nombre_lenght)
					+ sizeof(estLocalized->cantidad_coordenadas)
					+ (estLocalized->cantidad_coordenadas)*sizeof(estAppeared->coordenadas.posX)*sizeof(estAppeared->coordenadas.posY);
			a_enviar = malloc(*bytes + sizeof(*bytes));

			serializar_variable(a_enviar, bytes, sizeof(uint32_t), &offset);
			serializar_variable(a_enviar, &codigo_a_serializar, sizeof(codigo_a_serializar), &offset);
			serializar_variable(a_enviar, &id, sizeof(id), &offset);
			serializar_variable(a_enviar, &id_correlativo, sizeof(id_correlativo), &offset);
			serializar_nombre(a_enviar, estLocalized->nombre_pokemon, &offset);

			serializar_variable(a_enviar, &(estLocalized->cantidad_coordenadas), sizeof(estLocalized->cantidad_coordenadas), &offset);
			for(int i = 0; i < estLocalized->cantidad_coordenadas; i++) {
				serializar_variable(a_enviar, &(estLocalized->coordenadas[i].posX), sizeof(estLocalized->coordenadas[i].posX), &offset);
				serializar_variable(a_enviar, &(estLocalized->coordenadas[i].posY), sizeof(estLocalized->coordenadas[i].posY), &offset);
			}

			break;
		case CATCH_POKEMON: ;
			codigo_a_serializar = 3;
			t_catchPokemon_msg* estCatch = estructura;
			*bytes += estCatch->nombre_pokemon.nombre_lenght
					+ sizeof(estCatch->nombre_pokemon.nombre_lenght)
					+ sizeof(estCatch->coordenadas.posX)
					+ sizeof(estCatch->coordenadas.posY);
			a_enviar = malloc(*bytes + sizeof(*bytes));

			serializar_variable(a_enviar, bytes, sizeof(uint32_t), &offset);
			serializar_variable(a_enviar, &codigo_a_serializar, sizeof(codigo_a_serializar), &offset);
			serializar_variable(a_enviar, &id, sizeof(id), &offset);
			serializar_variable(a_enviar, &id_correlativo, sizeof(id_correlativo), &offset);
			serializar_nombre(a_enviar, estCatch->nombre_pokemon, &offset);
			serializar_variable(a_enviar, &(estCatch->coordenadas.posX), sizeof(estCatch->coordenadas.posX), &offset);
			serializar_variable(a_enviar, &(estCatch->coordenadas.posY), sizeof(estCatch->coordenadas.posY), &offset);
			break;
		case CAUGHT_POKEMON: ;
			codigo_a_serializar = 4;
			t_caughtPokemon_msg* estCaught = estructura;
			*bytes += sizeof(estCaught->atrapado);
			a_enviar = malloc(*bytes + sizeof(*bytes));

			serializar_variable(a_enviar, bytes, sizeof(uint32_t), &offset);
			serializar_variable(a_enviar, &codigo_a_serializar, sizeof(codigo_a_serializar), &offset);
			serializar_variable(a_enviar, &id, sizeof(id), &offset);
			serializar_variable(a_enviar, &id_correlativo, sizeof(id_correlativo), &offset);
			serializar_variable(a_enviar, &(estCaught->atrapado), sizeof(estCaught->atrapado), &offset);
			break;
		default: printf("\n[!] Error en el codigo de operacion al serializar paquete.\n"); break;
	}

	return a_enviar;
}

void serializar_variable(void* a_enviar, void* a_serializar, int tamanio, int *offset)
{
	memcpy(a_enviar + *offset, a_serializar, tamanio);
	*offset += tamanio;
}

void serializar_nombre(void* aEnviar, t_nombrePokemon nombrePokemon, int *offset)
{
	serializar_variable(aEnviar, &(nombrePokemon.nombre_lenght), sizeof(nombrePokemon.nombre_lenght), offset);
	serializar_variable(aEnviar, nombrePokemon.nombre, nombrePokemon.nombre_lenght, offset);
}


/////////////////////////////////////
// ---- Recepción de mensajes ---- //
/////////////////////////////////////

uint32_t obtener_cantidad_bytes_a_recibir(int socket_cliente)
{
	uint32_t bytes = 0;
	recv(socket_cliente, &bytes, sizeof(bytes), MSG_WAITALL); //TODO validar recv -1 y validar en func que lo usen
	return bytes;
}

t_paquete* recibir_paquete(int socket_cliente, char** nombre_recibido, uint32_t* tamanio_recibido)
{
	uint32_t bytes = obtener_cantidad_bytes_a_recibir(socket_cliente);
	*tamanio_recibido = bytes;

	t_paquete* paquete_recibido = malloc(sizeof(*paquete_recibido));
	void* stream = malloc(bytes);
	int status = recv(socket_cliente, stream, bytes, MSG_WAITALL);

	if (status <= 0) {
		free(stream);
		return NULL;
	}

	int offset = 0;
	deserializar_paquete(stream, paquete_recibido, &offset, bytes, nombre_recibido);

	free(stream);
	return(paquete_recibido);
}

void deserializar_paquete(void* stream, t_paquete* paquete_recibido, int* offset, uint32_t bytes, char** nombre_recibido)
{
	uint32_t codigo_deserializado = -1;
	copiar_variable(&(codigo_deserializado), stream, offset, sizeof(codigo_deserializado));

	switch(codigo_deserializado)
	{
		case 1: paquete_recibido->codigo_operacion = NEW_POKEMON; break;
		case 2: paquete_recibido->codigo_operacion = APPEARED_POKEMON; break;
		case 3: paquete_recibido->codigo_operacion = CATCH_POKEMON; break;
		case 4: paquete_recibido->codigo_operacion = CAUGHT_POKEMON; break;
		case 5: paquete_recibido->codigo_operacion = GET_POKEMON; break;
		case 6: paquete_recibido->codigo_operacion = LOCALIZED_POKEMON; break;
		case 7: paquete_recibido->codigo_operacion = SUSCRIPCION; break;
		default: paquete_recibido->codigo_operacion = ERROR_CODIGO; break;
	}


	copiar_variable(&(paquete_recibido->id), stream, offset, sizeof(paquete_recibido->id));
	copiar_variable(&(paquete_recibido->id_correlativo), stream, offset, sizeof(paquete_recibido->id_correlativo));

	// Le resto a "bytes" los bytes que ya copie del stream
	paquete_recibido->mensaje = malloc(bytes - *offset);

	switch(paquete_recibido->codigo_operacion)
	{
		case SUSCRIPCION: ;
			t_suscripcion_msg* estSuscripcion = malloc(sizeof(*estSuscripcion));

			copiar_variable(&(estSuscripcion->id_proceso), stream, offset, sizeof(estSuscripcion->id_proceso));
			copiar_variable(&(estSuscripcion->tipo_cola), stream, offset, sizeof(estSuscripcion->tipo_cola));
			copiar_variable(&(estSuscripcion->temporal), stream, offset, sizeof(estSuscripcion->temporal));

			*nombre_recibido = NULL;
			paquete_recibido->mensaje = estSuscripcion;
			break;
		case NEW_POKEMON: ;
			t_newPokemon_msg* estructuraNew = malloc(sizeof(*estructuraNew));

			copiar_nombre(&(estructuraNew->nombre_pokemon), stream, offset);
			copiar_coordenadas(&(estructuraNew->coordenadas), stream, offset);
			copiar_variable(&(estructuraNew->cantidad_pokemons), stream, offset, sizeof(estructuraNew->cantidad_pokemons));

			*nombre_recibido = estructuraNew->nombre_pokemon.nombre;
			paquete_recibido->mensaje = estructuraNew;

			break;
		case APPEARED_POKEMON: ;
			t_appearedPokemon_msg* estructuraAppeared = malloc(sizeof(*estructuraAppeared));

			copiar_nombre(&(estructuraAppeared->nombre_pokemon), stream, offset);
			copiar_coordenadas(&(estructuraAppeared->coordenadas), stream, offset);

			*nombre_recibido = estructuraAppeared->nombre_pokemon.nombre;
			paquete_recibido->mensaje = estructuraAppeared;
			break;
		case GET_POKEMON: ;
			t_getPokemon_msg* estructuraGet = malloc(sizeof(*estructuraGet));

			copiar_nombre(&(estructuraGet->nombre_pokemon), stream, offset);

			*nombre_recibido = estructuraGet->nombre_pokemon.nombre;
			paquete_recibido->mensaje = estructuraGet;
			break;
		case LOCALIZED_POKEMON: ;
			t_localizedPokemon_msg* estructuraLocalized = malloc(sizeof(*estructuraLocalized));

			copiar_nombre(&(estructuraLocalized->nombre_pokemon), stream, offset);
			copiar_variable(&(estructuraLocalized->cantidad_coordenadas), stream, offset, sizeof(estructuraLocalized->cantidad_coordenadas));
			estructuraLocalized->coordenadas = malloc( 2 * (estructuraLocalized->cantidad_coordenadas) * sizeof(uint32_t) );
			copiar_variable(estructuraLocalized->coordenadas, stream, offset, 2 * (estructuraLocalized->cantidad_coordenadas) * sizeof(uint32_t));

			*nombre_recibido = estructuraLocalized->nombre_pokemon.nombre;
			paquete_recibido->mensaje = estructuraLocalized;
			break;
		case CATCH_POKEMON: ;
			t_catchPokemon_msg* estructuraCatch = malloc(sizeof(*estructuraCatch));

			copiar_nombre(&(estructuraCatch->nombre_pokemon), stream, offset);
			copiar_coordenadas(&(estructuraCatch->coordenadas), stream, offset);

			*nombre_recibido = estructuraCatch->nombre_pokemon.nombre;
			paquete_recibido->mensaje = estructuraCatch;
			break;
		case CAUGHT_POKEMON: ;
			t_caughtPokemon_msg* estructuraCaught = malloc(sizeof(*estructuraCaught));
			copiar_variable(&(estructuraCaught->atrapado), stream, offset, sizeof(estructuraCaught->atrapado));

			*nombre_recibido = NULL;
			paquete_recibido->mensaje = estructuraCaught;
			break;
		default: printf("\n[!] Error en el codigo de operacion al deserializar paquete.\n"); break;
	}
}

void copiar_nombre(t_nombrePokemon* estructuraNombre, void* stream, int* offset)
{
	copiar_variable(&(estructuraNombre->nombre_lenght), stream, offset, sizeof(estructuraNombre->nombre_lenght));
	estructuraNombre->nombre = malloc(estructuraNombre->nombre_lenght);
	copiar_variable(estructuraNombre->nombre, stream, offset, estructuraNombre->nombre_lenght);
}

void copiar_coordenadas(t_coordenadas* estructuraCoordenadas, void* stream, int* offset)
{
	copiar_variable(&(estructuraCoordenadas->posX), stream, offset, sizeof(estructuraCoordenadas->posX));
	copiar_variable(&(estructuraCoordenadas->posY), stream, offset, sizeof(estructuraCoordenadas->posY));
}

void copiar_variable(void* variable, void* stream, int* offset, int size)
{
	memcpy(variable, stream + *offset, size);
	*offset += size;
}

void free_paquete_recibido(char* nombre_recibido, t_paquete* paquete_recibido)
{
// En principio se liberaba el nombre recibido y el mensaje pero esto termino generando problemas
//	if(nombre_recibido != NULL) {
//		free(nombre_recibido);
//	}
//	free(paquete_recibido->mensaje);
	free(paquete_recibido);
}

/////////////////////////////////
// ---- Respuesta mensaje ---- //
/////////////////////////////////

void enviar_id_respuesta(uint32_t id_msg, int socket_cliente)
{
	int offset = 0;
	void* a_enviar = malloc(sizeof(id_msg));
	serializar_variable(a_enviar, &id_msg, sizeof(id_msg), &offset);

	send(socket_cliente, a_enviar, sizeof(id_msg), 0);

	free(a_enviar);
}

uint32_t recibir_id(int socket_cliente)
{
	uint32_t id;
	recv(socket_cliente, &id, sizeof(id), MSG_WAITALL);//TODO validar recv -1
	return id;
}

int informar_ack(int socket_server)
{
	int offset = 0;
	uint32_t ack_code = 200;
	void* a_enviar = malloc(sizeof(ack_code));
	serializar_variable(a_enviar, &ack_code, sizeof(ack_code), &offset);

	int status = send(socket_server, a_enviar, sizeof(ack_code), 0);
	free(a_enviar);

	return status;
}

/////////////////////////////
// ---- Suscripciones ---- //
/////////////////////////////

int suscribirse_a_cola(t_suscripcion_msg* estructuraSuscripcion, int socket_servidor)
{
	return enviar_mensaje(SUSCRIPCION, 0, 0, (void*) estructuraSuscripcion, socket_servidor);
}

int desuscribirse_de_cola(int socket_servidor)
{
	uint32_t codigo = 408;
	return send(socket_servidor, &codigo, sizeof(codigo), 0);
}

int respuesta_suscripcion_cantidad_y_tamanio(uint32_t* cantidad_paquetes, uint32_t* tamanio_stream, int socket_servidor)
{
	void* recibido = malloc(sizeof(uint32_t)*2);
	int status = recv(socket_servidor, recibido, sizeof(uint32_t)*2, MSG_WAITALL);
	memcpy(cantidad_paquetes, recibido, sizeof(uint32_t));
	memcpy(tamanio_stream, recibido + sizeof(uint32_t), sizeof(uint32_t));
	free(recibido);
	return status;
}

t_list* respuesta_suscripcion_obtener_paquetes(int socket_servidor, uint32_t* cant_paquetes_recibidos)
{
	t_list* paquetes = list_create();
	uint32_t cantidad_paquetes;
	uint32_t tamanio_stream;

	respuesta_suscripcion_cantidad_y_tamanio(&cantidad_paquetes, &tamanio_stream, socket_servidor);
	*cant_paquetes_recibidos = cantidad_paquetes;

	if (cantidad_paquetes > 0) {
		void* stream = malloc(tamanio_stream);
		recv(socket_servidor, stream, tamanio_stream, MSG_WAITALL);

		int offset = 0;
		while (cantidad_paquetes > 0) {
			char* nombre_recibido = NULL;
			t_paquete* paquete_recibido = malloc(sizeof(*paquete_recibido));
			uint32_t bytes;
			memcpy(&bytes, stream + offset, sizeof(bytes));
			offset += sizeof(bytes);

			deserializar_paquete(stream, paquete_recibido, &offset, bytes, &nombre_recibido);

			list_add(paquetes, (void*) paquete_recibido);
			cantidad_paquetes--;
		}
		free(stream);
	}

	return paquetes;
}

/////////////////////
// ---- Otros ---- //
/////////////////////

void liberar_conexion(int socket)
{
	close(socket);
}

