/*
 ============================================================================
 Name        : GameBoy
 Author      : Fran and Co
 Description : Proceso GameBoy
 ============================================================================
 */

#include "gameboy.h"

int main(int argc, char *argv[])
{
	config = leer_config();
	LOGGER = iniciar_logger();

	if(leer_config() == NULL || iniciar_logger() == NULL)
		return -1;

	char* ip;
	char* puerto;
	process_code codigoProceso;
	op_code codigoOperacion;
	int status = 0;

	chequearSiEsSuscripcion(argv[1], argv[2], &codigoOperacion, &codigoProceso);

	if(asignarDatosConexion(config, &ip, &puerto, codigoProceso) == -1)
		return -1;

	int socket_cliente = crear_conexion(ip, puerto);

	if(socket_cliente == -1)
		return -1;

	log_conexion_a_proceso(ip, puerto);

	uint32_t idAEnviar = 0;
	uint32_t idCorrAEnviar = 0;
	switch(codigoOperacion)
	{
		case NEW_POKEMON: ;
			t_newPokemon_msg estructuraNew;
			estructuraNew.nombre_pokemon.nombre = argv[3];
			estructuraNew.nombre_pokemon.nombre_lenght = strlen(estructuraNew.nombre_pokemon.nombre);
			estructuraNew.coordenadas.posX = atoi(argv[4]);
			estructuraNew.coordenadas.posY = atoi(argv[5]);
			estructuraNew.cantidad_pokemons = atoi(argv[6]);
			if (codigoProceso == GAMECARD) {
				idAEnviar = atoi(argv[7]);
			}
			status = enviar_mensaje(codigoOperacion, idAEnviar, 0, &estructuraNew, socket_cliente);
			break;
		case APPEARED_POKEMON: ;
			t_appearedPokemon_msg estructuraAppeared;
			estructuraAppeared.nombre_pokemon.nombre = argv[3];
			estructuraAppeared.nombre_pokemon.nombre_lenght = strlen(estructuraAppeared.nombre_pokemon.nombre);
			estructuraAppeared.coordenadas.posX = atoi(argv[4]);
			estructuraAppeared.coordenadas.posY = atoi(argv[5]);
			if (codigoProceso == BROKER) {
				idCorrAEnviar =  atoi(argv[6]);
			}
			status = enviar_mensaje(codigoOperacion, 0, idCorrAEnviar, &estructuraAppeared, socket_cliente);
			break;
		case GET_POKEMON: ;
			t_getPokemon_msg estructuraGet;
			estructuraGet.nombre_pokemon.nombre = argv[3];
			estructuraGet.nombre_pokemon.nombre_lenght = strlen(estructuraGet.nombre_pokemon.nombre);
			if (codigoProceso == GAMECARD) {
				idAEnviar = atoi(argv[4]);
			}
			status = enviar_mensaje(codigoOperacion, idAEnviar, 0, &estructuraGet, socket_cliente);
			break;
		case LOCALIZED_POKEMON: ;
			t_localizedPokemon_msg estructuraLocalized;
			estructuraLocalized.nombre_pokemon.nombre = argv[3];
			estructuraLocalized.nombre_pokemon.nombre_lenght = strlen(estructuraLocalized.nombre_pokemon.nombre);
			estructuraLocalized.cantidad_coordenadas = atoi(argv[4]);
			estructuraLocalized.coordenadas = malloc(sizeof(uint32_t) * estructuraLocalized.cantidad_coordenadas * 2);
			int j = 5;
			for(int i = 0; i < estructuraLocalized.cantidad_coordenadas; i++)
			{
				estructuraLocalized.coordenadas[i].posX = atoi(argv[j]);
				estructuraLocalized.coordenadas[i].posY = atoi(argv[j+1]);
				j+=2;
			}
			status = enviar_mensaje(codigoOperacion, 0, 0, &estructuraLocalized, socket_cliente);
			break;
		case CATCH_POKEMON: ;
			t_catchPokemon_msg estructuraCatch;
			estructuraCatch.nombre_pokemon.nombre = argv[3];
			estructuraCatch.nombre_pokemon.nombre_lenght = strlen(estructuraCatch.nombre_pokemon.nombre);
			estructuraCatch.coordenadas.posX = atoi(argv[4]);
			estructuraCatch.coordenadas.posY = atoi(argv[5]);
			if (codigoProceso == GAMECARD) {
				idAEnviar = atoi(argv[6]);
			}
			status = enviar_mensaje(codigoOperacion, idAEnviar, 0, &estructuraCatch, socket_cliente);
			break;
		case CAUGHT_POKEMON: ;
			t_caughtPokemon_msg estructuraCaught;
			uint32_t atrapado = fueAtrapado(argv[4]);
			estructuraCaught.atrapado = atrapado;
			status = enviar_mensaje(codigoOperacion, 0, atoi(argv[3]), &estructuraCaught, socket_cliente);
			break;
		case SUSCRIPCION: ;
			uint32_t tiempo = (uint32_t) atoi(argv[3]);
			suscribirse(argv[2], tiempo, socket_cliente);
			break;
		case ERROR_CODIGO: return -1; break;
		default: return -1; break;
	}

	if (status >= 0 && codigoProceso == BROKER && codigoOperacion != SUSCRIPCION) {
		recibir_id(socket_cliente);
	}

	terminar_programa(socket_cliente, LOGGER, config);
	return EXIT_SUCCESS;
}

void suscribirse(const char* tipo_cola, uint32_t tiempo, int socket_broker)
{
	t_suscripcion_msg* estructuraSuscripcion = malloc(sizeof(*estructuraSuscripcion));
	estructuraSuscripcion->id_proceso = atoi(config_get_string_value(config, "ID_PROCESO"));
	estructuraSuscripcion->tipo_cola = stringACodigoOperacion(tipo_cola);
	estructuraSuscripcion->temporal = tiempo;

	int status_susc = suscribirse_a_cola(estructuraSuscripcion, socket_broker);

	if (status_susc >= 0) {
		uint32_t cant_paquetes;
		t_list* paquetes = respuesta_suscripcion_obtener_paquetes(socket_broker, &cant_paquetes);

		informar_ack(socket_broker);

		for(int i=0; i < cant_paquetes; i++) {
			t_paquete* paquete_recibido = list_get(paquetes, i);
			log_nuevo_mensaje(tipo_cola, paquete_recibido->id, paquete_recibido->id_correlativo);
			free(paquete_recibido->mensaje);
			free(paquete_recibido);
		}

		free(estructuraSuscripcion);
		list_destroy(paquetes);

		while(1) {
			char* nombre_recibido = NULL;
			uint32_t size = 0;
			t_paquete* paquete_recibido = recibir_paquete(socket_broker, &nombre_recibido, &size);

			if (paquete_recibido == NULL) {
				break;
			}
			log_nuevo_mensaje(tipo_cola, paquete_recibido->id, paquete_recibido->id_correlativo);

			informar_ack(socket_broker);
			free(paquete_recibido->mensaje);
			free(paquete_recibido);
			break;
		}
	}

}

t_log* iniciar_logger(void)
{
	return log_create(GAMEBOY_LOG, GAMEBOY_NAME, false, LOG_LEVEL_INFO);
}

t_config* leer_config(void)
{
	return config_create(GAMEBOY_CONFIG);
}

op_code stringACodigoOperacion(const char* string)
{
	for(int i = 0; i < sizeof(conversionCodigoOp) / sizeof(conversionCodigoOp[0]); i++)
	{
		if(!strcmp(string, conversionCodigoOp[i].str))
			return conversionCodigoOp[i].codigoOperacion;
	}
	return ERROR_CODIGO;
}

process_code stringACodigoProceso(const char* string)
{
	for(int i = 0; i < sizeof(conversionCodigoProceso) / sizeof(conversionCodigoProceso[0]); i++)
	{
		if(!strcmp(string, conversionCodigoProceso[i].str))
			return conversionCodigoProceso[i].codigoProceso;
	}
	return ERROR_PROCESO;
}

uint32_t fueAtrapado(const char* estadoAtrapado)
{
	if(strcmp(estadoAtrapado, "OK") == 0) {
		return 1;
	} else if (strcmp(estadoAtrapado, "FAIL") == 0) {
		return 0;
	}
	return 0;
}

void chequearSiEsSuscripcion(const char* argumento1, const char* argumento2, op_code* codigoOperacion, process_code* codigoProceso)
{
	if(strcmp(argumento1, "SUSCRIPTOR") == 0)
	{
		*codigoProceso = BROKER;
		*codigoOperacion = SUSCRIPCION;
	}
	else
	{
		*codigoProceso = stringACodigoProceso(argumento1);
		*codigoOperacion = stringACodigoOperacion(argumento2);
	}
}

int asignarDatosConexion(t_config* config, char** ip, char** puerto, process_code codigoProceso)
{
	switch(codigoProceso)
	{
		case BROKER:
			*ip = config_get_string_value(config, "IP_BROKER");
			*puerto = config_get_string_value(config, "PUERTO_BROKER");
			break;
		case TEAM:
			*ip = config_get_string_value(config, "IP_TEAM");
			*puerto = config_get_string_value(config, "PUERTO_TEAM");
			break;
		case GAMECARD:
			*ip = config_get_string_value(config, "IP_GAMECARD");
			*puerto = config_get_string_value(config, "PUERTO_GAMECARD");
			break;
		case ERROR_PROCESO: return -1; break;
		default: return -1; break;
	}
	return 0;
}

//TODO
void terminar_programa(int conexion, t_log* logger, t_config* config)
{
	config_destroy(config);
	log_destroy(logger);
	liberar_conexion(conexion);
}
