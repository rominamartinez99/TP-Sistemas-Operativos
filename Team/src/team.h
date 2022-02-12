/*
 ============================================================================
 Name        : Team
 Author      : Fran and Co
 Description : Proceso Team Header
 ============================================================================
*/

#ifndef TEAM_H_
#define TEAM_H_

#include "funcionesUtilesTeam.h"

// IP y PUERTO de team para iniciar servidor

#define TEAM_LOG "team.log"
#define TEAM_NAME "team"
#define TEAM_CONFIG "team.config"

const static struct {
	op_code codigoOperacion;
	const char* str;
} conversionCodigoOp[] = {

		{CATCH_POKEMON, "CATCH_POKEMON"},
		{GET_POKEMON, "GET_POKEMON"},

};

pthread_t thread;
pthread_t threadEscucha;

void quedarseALaEscucha();

t_log* iniciar_logger(void);

t_config* leer_config(void);

void inicializarConfig();

void inicializarSemaforoPlanificador();

void suscribirseAppeared();

void suscribirseCaught();

void suscribirseLocalized();

void suscribirseAColas();

void suscribirseA(op_code);

void serve_client(int*);

void process_request(char*, t_paquete*);

char* arreglarNombrePokemon(t_nombrePokemon);

bool especieEstaEnLista(t_list*, char*, pthread_mutex_t);

op_code stringACodigoOperacion(const char*);

void enviarMensajeGetABroker();

t_list* eliminarRepetidos();

void enviarMensajeGet(t_nombrePokemon*);

void inicializarListas();

void esperarIdGet(int);

void requiere(t_appearedPokemon_msg*);

int necesitaTeamAlPokemon(t_nombrePokemon*);

//void terminar_programa(int socket, t_log* logger, t_config* config);

#endif /* TEAM_H_ */
