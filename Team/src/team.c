/*
 ============================================================================
 Name        : Team
 Author      : Fran and Co
 Description : Proceso Team
 ============================================================================
*/

#include "team.h"

int main(void) {

	inicializarSemaforoPlanificador();
	inicializarListas();
	config = leer_config();
	inicializarConfig();

	inicioAlgoritmoDeadlock = 0;

	ponerEntrenadoresEnLista();
	crearHilosEntrenadores();

	enviarMensajeGetABroker();

	suscribirseAColas();

	printf("team!\n");

	quedarseALaEscucha();

	return EXIT_SUCCESS;
}

void quedarseALaEscucha() {
	int socket_servidor = iniciar_servidor(IP_TEAM, PUERTO_TEAM);
	list_add(socketsALiberar, &socket_servidor);
	while(1) {
		int socket_potencial = esperar_cliente(socket_servidor);
		if(socket_potencial > 0) {
			int* socket_cliente = (int*) malloc(sizeof(int));
			*socket_cliente = socket_potencial;
			pthread_create(&threadEscucha,NULL,(void*)serve_client,socket_cliente);
			pthread_detach(threadEscucha);
		}
	}
}

t_log* iniciar_logger(void)
{
	return log_create(TEAM_LOG, TEAM_NAME, true, LOG_LEVEL_INFO);
}

t_config* leer_config(void)
{
	return config_create(TEAM_CONFIG);

}

void inicializarConfig(){

	IP_TEAM = config_get_string_value(config, "IP_TEAM");
	PUERTO_TEAM = config_get_string_value(config, "PUERTO_TEAM");
	ID_TEAM = config_get_int_value(config, "ID");
	TIEMPO_RECONEXION = config_get_int_value(config, "TIEMPO_RECONEXION");
	ipBroker = config_get_string_value(config, "IP_BROKER");
	puertoBroker = config_get_string_value(config, "PUERTO_BROKER");
	algoritmoPlanificacion = config_get_string_value(config,"ALGORITMO_PLANIFICACION");
	quantum= config_get_int_value(config, "QUANTUM");
	estimacionInicial= config_get_int_value(config, "ESTIMACION_INICIAL");
	alfa = config_get_double_value(config, "ALPHA");
	retardoCPU = config_get_int_value(config, "RETARDO_CICLO_CPU");
	char* team_log = config_get_string_value(config, "LOG_FILE");
	LOGGER = log_create(team_log, PUERTO_TEAM, false, LOG_LEVEL_INFO);

}

void inicializarSemaforoPlanificador(){
	sem_init(&sem_planificar, 0, 1);
}

void suscribirseAppeared(){
	suscribirseA(APPEARED_POKEMON);
}

void suscribirseCaught(){
	suscribirseA(CAUGHT_POKEMON);
}

void suscribirseLocalized(){
	suscribirseA(LOCALIZED_POKEMON);
}

void suscribirseAColas(){

	pthread_create(&thread, NULL, (void*)suscribirseAppeared, NULL);
	pthread_detach(thread);
	pthread_create(&thread, NULL, (void*)suscribirseCaught, NULL);
	pthread_detach(thread);
	pthread_create(&thread, NULL, (void*)suscribirseLocalized, NULL);
	pthread_detach(thread);

}

void suscribirseA(op_code tipo_cola){

	pthread_mutex_lock(&mutex_send);
	int socket_cliente = crear_conexion(ipBroker, puertoBroker);

	list_add(socketsALiberar, &socket_cliente);

	while(socket_cliente<=0){
		log_inicio_reintento_conexion_broker();
		sleep(TIEMPO_RECONEXION);
		socket_cliente = crear_conexion(ipBroker, puertoBroker);
		log_resultado_proceso_reintento_conexion_broker(socket_cliente);
	}

	t_suscripcion_msg* estructuraSuscripcion = malloc(sizeof(t_suscripcion_msg));
	estructuraSuscripcion->id_proceso = ID_TEAM;
	estructuraSuscripcion->tipo_cola = tipo_cola;
	estructuraSuscripcion->temporal = 0;

	suscribirse_a_cola(estructuraSuscripcion, socket_cliente);
	pthread_mutex_unlock(&mutex_send);

	uint32_t cant_paquetes;
	t_list* paquetes = respuesta_suscripcion_obtener_paquetes(socket_cliente, &cant_paquetes);
	fflush(stdout);

	informar_ack(socket_cliente);

	for(int i = 0; i<cant_paquetes; i++){
		t_paquete* paquete_recibido = list_get(paquetes, i);
		process_request(NULL, paquete_recibido);
	}

	free(estructuraSuscripcion);

	while(1){
		char*nombre_recibido = NULL;
		uint32_t tamanioRecibido;

		t_paquete* paquete_recibido = recibir_paquete(socket_cliente,&nombre_recibido, &tamanioRecibido);

		if(paquete_recibido == NULL){
			log_inicio_reintento_conexion_broker();
			sleep(TIEMPO_RECONEXION);
			suscribirseA(tipo_cola);
			log_resultado_proceso_reintento_conexion_broker(socket_cliente);
		}

		process_request(nombre_recibido, paquete_recibido);

		informar_ack(socket_cliente);
	}

}

void serve_client(int* socket_cliente)
{
	char* nombre_recibido = NULL;
	uint32_t tamanioRecibido;

	t_paquete* paquete_recibido = recibir_paquete(*socket_cliente, &nombre_recibido, &tamanioRecibido);

	process_request(nombre_recibido, paquete_recibido);

	close(*socket_cliente);
}

void process_request(char* nombre_recibido, t_paquete* paquete_recibido)
{
	char* nombrePosta;
	switch(paquete_recibido->codigo_operacion)
	{
		case APPEARED_POKEMON: ;

			t_appearedPokemon_msg* mensajeAppeared = (t_appearedPokemon_msg*) paquete_recibido->mensaje;

			nombrePosta = arreglarNombrePokemon(mensajeAppeared->nombre_pokemon);

			free(mensajeAppeared->nombre_pokemon.nombre);

			mensajeAppeared->nombre_pokemon.nombre = nombrePosta;

			log_llegada_appeared(paquete_recibido->id_correlativo, mensajeAppeared->nombre_pokemon.nombre,
					mensajeAppeared->coordenadas.posX,
					mensajeAppeared->coordenadas.posY);

			if (!especieEstaEnLista(especiesQueLlegaron, mensajeAppeared->nombre_pokemon.nombre, mutex_especies_que_llegaron)) {
				pthread_mutex_lock(&mutex_especies_que_llegaron);
				list_add(especiesQueLlegaron, &(mensajeAppeared->nombre_pokemon.nombre));
				pthread_mutex_unlock(&mutex_especies_que_llegaron);
			}

			requiere(mensajeAppeared);

			free_paquete_recibido(nombre_recibido, paquete_recibido);

		break;

		case LOCALIZED_POKEMON: ;

			t_localizedPokemon_msg* mensajeLocalized = (t_localizedPokemon_msg*) paquete_recibido->mensaje;

			nombrePosta = arreglarNombrePokemon(mensajeLocalized->nombre_pokemon);

			free(mensajeLocalized->nombre_pokemon.nombre);

			mensajeLocalized->nombre_pokemon.nombre = nombrePosta;

			char* coordenadas =  string_new();
			if (mensajeLocalized->cantidad_coordenadas == 0) {
				string_append(&coordenadas, "[  ]");
			}

			for(int i=0; i<(mensajeLocalized->cantidad_coordenadas); i++){

				char* x = string_itoa(mensajeLocalized->coordenadas[i].posX);
				char* y = string_itoa(mensajeLocalized->coordenadas[i].posY);

				string_append(&coordenadas, "[");
				string_append(&coordenadas, x);
				string_append(&coordenadas, ";");
				string_append(&coordenadas, y);
				string_append(&coordenadas, "]");
				string_append(&coordenadas, " ");
			}
			log_llegada_localized(paquete_recibido->id, mensajeLocalized->nombre_pokemon.nombre, mensajeLocalized->cantidad_coordenadas, coordenadas);

			bool compararId(void* elemento){
				uint32_t* id = (uint32_t*) elemento;
				return *id == (paquete_recibido->id_correlativo);
			}

			if(
				!especieEstaEnLista(especiesQueLlegaron, mensajeLocalized->nombre_pokemon.nombre, mutex_especies_que_llegaron)
				&& list_any_satisfy(id_mensajeGet, compararId)
				&& necesitaTeamAlPokemon(&(mensajeLocalized->nombre_pokemon))
			){
					pthread_mutex_lock(&mutex_especies_que_llegaron);
					list_add(especiesQueLlegaron, &(mensajeLocalized->nombre_pokemon.nombre));
					pthread_mutex_unlock(&mutex_especies_que_llegaron);

					pthread_mutex_lock(&mutex_mensajesLocalized);
					list_add(mensajesLocalized, paquete_recibido);
					pthread_mutex_unlock(&mutex_mensajesLocalized);

					int cuantosNecesita = necesitaTeamAlPokemon(&(mensajeLocalized->nombre_pokemon));

					for(int i=0; i<cuantosNecesita && mensajeLocalized->cantidad_coordenadas > 0; i++){
						sem_wait(&sem_buscarEntrenadorMasCercano);
						buscarPokemonLocalized(mensajeLocalized, paquete_recibido->id);
						planificarSegun();

					} //Si necesita + de un pikachu los manda a buscar.
			}

			break;

		case CAUGHT_POKEMON: ;

		t_caughtPokemon_msg* mensajeCaught = (t_caughtPokemon_msg*) paquete_recibido->mensaje;

		log_llegada_caught(paquete_recibido->id_correlativo, mensajeCaught->atrapado);

		int a = list_size(entrenadores);
		for(int i=0; i<a; i++){
			t_entrenador* entrenador = list_get(entrenadores, i);
			if(entrenador->idMensajeCaught == paquete_recibido->id_correlativo){
				if(mensajeCaught->atrapado){
					entrenador->puedeAtrapar = 1;
					entrenador->esLocalized = 0;
					planificarCaught();
				} else {

					entrenador->puedeAtrapar = 0;
					entrenador->idMensajeCaught = 0;
					entrenador->pokemonInstantaneo = NULL;

					if(entrenador->esLocalized){

						bool esElMensaje(void* elemento){
							t_paquete* paquete= (t_paquete*) elemento;

							return (entrenador->esLocalized) == (paquete->id);
						}

						pthread_mutex_lock(&mutex_mensajesLocalized);
						t_paquete* paquete = list_find(mensajesLocalized, esElMensaje);
						pthread_mutex_unlock(&mutex_mensajesLocalized);

						sem_wait(&sem_buscarEntrenadorMasCercano);
						buscarPokemonLocalized(paquete->mensaje, paquete->id);
						planificarSegun();
					} else {
						planificarCaught();
					}
				}
			}
		}

		free_paquete_recibido(nombre_recibido, paquete_recibido);

			break;

		default:

			break;
	}
}

char* arreglarNombrePokemon(t_nombrePokemon nombrePokemon)
{
	char* nombreAUsar = malloc(nombrePokemon.nombre_lenght+1);
	memcpy(nombreAUsar, nombrePokemon.nombre, nombrePokemon.nombre_lenght);
	char caracterNulo = '\0';
	memcpy(nombreAUsar+nombrePokemon.nombre_lenght, &caracterNulo, 1);
	return nombreAUsar;
}

bool especieEstaEnLista(t_list* list, char* nombrePokemon, pthread_mutex_t mutex) {

	bool esMismaEspecie(void* elemento){
		char* nombrePokemonLista = (char*) elemento;
		return string_equals_ignore_case(nombrePokemonLista, nombrePokemon);
	}

	pthread_mutex_lock(&mutex);
	bool estaEnLista = list_any_satisfy(list, esMismaEspecie);
	pthread_mutex_unlock(&mutex);

	return estaEnLista;
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

void enviarMensajeGetABroker(){

	t_list* objetivoTeamSinRepe = eliminarRepetidos();

	int tamanioObjTeamSinRepetidos = list_size(objetivoTeamSinRepe);

	for(int a=0; a< tamanioObjTeamSinRepetidos ; a++){
		t_nombrePokemon* pokemon = (t_nombrePokemon*) list_get(objetivoTeamSinRepe, a);
		enviarMensajeGet(pokemon);

		pthread_mutex_lock(&mutex_especies_requeridas);
		list_add(especiesRequeridas, pokemon->nombre);
		pthread_mutex_unlock(&mutex_especies_requeridas);
	}

	list_destroy(objetivoTeamSinRepe);
}

t_list* eliminarRepetidos(){

	t_list* objetivoTeamSinRepetidos = list_create();
	int c=0;

	pthread_mutex_lock(&mutex_objetivoTeam);

	int a = list_size(objetivoTeam);

	for(int i=0; i < a; i++){

		int k=0;
		while((k<c) && (!sonIguales(list_get(objetivoTeamSinRepetidos,k), list_get(objetivoTeam, i)))){
			k++;
		}

		if(k==c){
			list_add(objetivoTeamSinRepetidos, (t_nombrePokemon*)list_get(objetivoTeam, i));
			c++;
		}
	}

	pthread_mutex_unlock(&mutex_objetivoTeam);

	return objetivoTeamSinRepetidos;
}

void enviarMensajeGet(t_nombrePokemon* pokemon){
	t_getPokemon_msg* estructuraPokemon = malloc(sizeof(t_nombrePokemon));
	estructuraPokemon->nombre_pokemon = *pokemon;
	int socket_cliente = crear_conexion(ipBroker, puertoBroker);
	int status = enviar_mensaje(GET_POKEMON, 0, 0, estructuraPokemon, socket_cliente);

	if(status>=0){
		esperarIdGet(socket_cliente);
	} else{
		log_error_comunicacion_con_broker();
	}

	free(estructuraPokemon);
	liberar_conexion(socket_cliente);
}

void inicializarListas(){
	id_mensajeGet = list_create();
	id_mensajeCatch = list_create();
	mensajesLocalized = list_create();
	atrapados = list_create();
	objetivoTeam = list_create();
	especiesRequeridas = list_create();
	especiesQueLlegaron = list_create();
	entrenadorIntercambio = list_create();
	entrenadorConQuienIntercambiar = list_create();
	entrenadoresNoSeleccionables = list_create();

	socketsALiberar = list_create();
}

void esperarIdGet(int socket_cliente){
	uint32_t* id_respuesta = malloc(sizeof(uint32_t));
	*id_respuesta = recibir_id(socket_cliente);
	pthread_mutex_lock(&mutex_id_mensaje_get);
	list_add(id_mensajeGet,(void*) id_respuesta);
	pthread_mutex_unlock(&mutex_id_mensaje_get);
}

void requiere(t_appearedPokemon_msg* mensajeAppeared){

	int j=0;
	pthread_mutex_lock(&mutex_pendientes);
	int a = list_size(pendientes);

	for(int i=0; i < a; i++){
		if(!sonIguales(&(mensajeAppeared->nombre_pokemon), list_get(pendientes, i))){
			j++;
		}
	}
	pthread_mutex_unlock(&mutex_pendientes);

	if(j!=a){
		t_newPokemon* pokemonNuevo = malloc(sizeof(t_newPokemon));
		pokemonNuevo->pokemon = &(mensajeAppeared->nombre_pokemon);
		pokemonNuevo->coordenadas = &(mensajeAppeared->coordenadas);

		sem_wait(&sem_buscarEntrenadorMasCercano);

		buscarPokemonAppeared(pokemonNuevo);
		planificarSegun();
	}
}

int necesitaTeamAlPokemon(t_nombrePokemon* pokemon){

	pthread_mutex_lock(&mutex_pendientes);

	int cantidadPokemonesDeEspecieQueNecesita = 0;

	int a = list_size(pendientes);

		for(int i=0; i < a; i++){

			if(sonIguales(pokemon, list_get(pendientes, i))){
				cantidadPokemonesDeEspecieQueNecesita++;
			}
		}

	pthread_mutex_unlock(&mutex_pendientes);

	return cantidadPokemonesDeEspecieQueNecesita;
}

