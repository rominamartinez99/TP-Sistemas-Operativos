/*
 ============================================================================
 Name        : GameCard
 Author      : Fran and Co
 Description : Proceso Game Card
 ============================================================================
 */

#include "gamecard.h"

char* ip;
char* puerto;
int tiempoReconexion;
t_log* logger;

int main(void) {

	configGeneral = setear_config();
	logger = log_create(GAMECARD_LOG, "gamecard.log", false, LOG_LEVEL_INFO);
	PUNTO_MONTAJE = config_get_string_value(configGeneral, "PUNTO_MONTAJE_TALLGRASS");

	configuracionInicial();

	t_suscripcion_msg datosHiloNP;
	datosHiloNP.id_proceso = config_get_int_value(configGeneral, "ID_HILO_NP");
	datosHiloNP.tipo_cola = NEW_POKEMON;
	datosHiloNP.temporal = 0;

	t_suscripcion_msg datosHiloGP;
	datosHiloGP.id_proceso = config_get_int_value(configGeneral, "ID_HILO_GP");
	datosHiloGP.tipo_cola = GET_POKEMON;
	datosHiloGP.temporal = 0;

	t_suscripcion_msg datosHiloCP;
	datosHiloCP.id_proceso = config_get_int_value(configGeneral, "ID_HILO_CP");
	datosHiloCP.tipo_cola = CATCH_POKEMON;
	datosHiloCP.temporal = 0;

	pthread_create(&threadNewPokemon, NULL, (void*)conectarseYSuscribirse, &datosHiloNP);
	pthread_create(&threadGetPokemon, NULL, (void*)conectarseYSuscribirse, &datosHiloGP);
	pthread_create(&threadCatchPokemon, NULL, (void*)conectarseYSuscribirse, &datosHiloCP);
	pthread_create(&threadMessages, NULL, (void*)esperarMensajes, NULL);

	pthread_join(threadNewPokemon, NULL);
	pthread_join(threadGetPokemon, NULL);
	pthread_join(threadCatchPokemon, NULL);

	config_destroy(configGeneral);
	return EXIT_SUCCESS;
}

t_config* setear_config(void)
{
	t_config* config = config_create(GAMECARD_CONFIG);
	ip = config_get_string_value(config, "IP_BROKER");
	puerto = config_get_string_value(config, "PUERTO_BROKER");
	tiempoReconexion =  config_get_int_value(config, "TIEMPO_DE_REINTENTO_CONEXION");
	return config;
}

void conectarseYSuscribirse(t_suscripcion_msg* datosHilo)
{
	fflush(stdout);
	int socket_cliente = crear_conexion(ip, puerto);

	while(socket_cliente < 0)
	{
		sleep(tiempoReconexion);
		socket_cliente = crear_conexion(ip, puerto);
	}

	suscribirse_a_cola(datosHilo, socket_cliente);

	recepcionMensajesDeCola(datosHilo, socket_cliente);
}

void recepcionMensajesDeCola(t_suscripcion_msg* datosHilo, int socket_cliente)
{
	uint32_t cant_paquetes;
	t_list* paquetes = respuesta_suscripcion_obtener_paquetes(socket_cliente, &cant_paquetes);

	informar_ack(socket_cliente);

	for(int i=0; i < cant_paquetes; i++)
	{
		t_paquete* paquete_recibido = list_get(paquetes, i);

		devolverMensajeCorrespondiente(paquete_recibido);

		free(paquete_recibido);
	}

	list_destroy(paquetes);

	while(1)
	{
		char* nombre_recibido = NULL;
		uint32_t tamanio_recibido;
		t_paquete* paquete_recibido = recibir_paquete(socket_cliente, &nombre_recibido, &tamanio_recibido);
		while(paquete_recibido == NULL)
		{
			sleep(tiempoReconexion);
			conectarseYSuscribirse(datosHilo);
		}

		informar_ack(socket_cliente);

		devolverMensajeCorrespondiente(paquete_recibido);

		free(paquete_recibido);
	}
}

void devolverMensajeCorrespondiente(t_paquete* paquete_recibido)
{
	int socketTemporal = crear_conexion(ip, puerto);
	op_code codigoOperacion = paquete_recibido->codigo_operacion;
	switch(codigoOperacion)
	{
		case NEW_POKEMON: ;
			t_newPokemon_msg* estructuraNew = (t_newPokemon_msg*) paquete_recibido->mensaje;


			t_appearedPokemon_msg estructuraAppeared = procesarNewPokemon(estructuraNew);

			if(chequearMensajeBroker(socketTemporal))
				enviar_mensaje(APPEARED_POKEMON, 0, paquete_recibido->id, &estructuraAppeared, socketTemporal);

			free(estructuraNew->nombre_pokemon.nombre);
			free(estructuraNew);
			break;
		case GET_POKEMON: ;
			t_getPokemon_msg* estructuraGet = (t_getPokemon_msg*) paquete_recibido->mensaje;

			t_localizedPokemon_msg estructuraLocalized = procesarGetPokemon(estructuraGet);

			if(chequearMensajeBroker(socketTemporal))
				enviar_mensaje(LOCALIZED_POKEMON, 0, paquete_recibido->id, &estructuraLocalized, socketTemporal);

			free(estructuraGet->nombre_pokemon.nombre);
			free(estructuraGet);
			free(estructuraLocalized.coordenadas);
			break;
		case CATCH_POKEMON: ;
			t_catchPokemon_msg* estructuraCatch = (t_catchPokemon_msg*) paquete_recibido->mensaje;

			t_caughtPokemon_msg estructuraCaught = procesarCatchPokemon(estructuraCatch);

			if(estructuraCaught.atrapado == -1)
			{
				log_error(logger, "El pokemon no existe en el FileSystem.");
				estructuraCaught.atrapado = 0;
			}
			else if(estructuraCaught.atrapado == -2)
			{
				log_error(logger, "El pokemon no se encuentra en la coordenada solicitada.");
				estructuraCaught.atrapado = 0;
			}

			if(chequearMensajeBroker(socketTemporal))
				enviar_mensaje(CAUGHT_POKEMON, 0, paquete_recibido->id, &estructuraCaught, socketTemporal);

			free(estructuraCatch->nombre_pokemon.nombre);
			free(estructuraCatch);
			break;
		default: break;
	}
	recibir_id(socketTemporal);
	liberar_conexion(socketTemporal);
}

void esperarMensajes(void)
{
	char* ipLocal = config_get_string_value(configGeneral, "IP_GAMECARD");
	char* puertoLocal = config_get_string_value(configGeneral, "PUERTO_GAMECARD");
	int socket_servidor = iniciar_servidor(ipLocal, puertoLocal);
	while(1) {
		int clientePotencial = esperar_cliente(socket_servidor);
		if(clientePotencial > 0) {
			int* socket_cliente = (int*) malloc(sizeof(int));
			*socket_cliente = clientePotencial;
			serve_client(socket_cliente);
		}
	}
}

void serve_client(int* client_socket)
{
	char* nombre_recibido = NULL;
	uint32_t size_message = 0;
	t_paquete* paquete_recibido = recibir_paquete(*client_socket, &nombre_recibido, &size_message);

	devolverMensajeCorrespondiente(paquete_recibido);

	free(paquete_recibido);
}

bool chequearMensajeBroker(int socketTemporal)
{
	if(socketTemporal<=0)
	{
		log_error(logger, "No se pudo enviar el mensaje a broker.");
		return false;
	}
	else
		return true;
}
