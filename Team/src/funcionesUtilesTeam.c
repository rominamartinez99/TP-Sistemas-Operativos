/*
 * funcionesUtilesTeam.c
 *
 *  Created on: 4 may. 2020
 *      Author: utnso
 */

#include "funcionesUtilesTeam.h"

pthread_mutex_t mutex_send = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_id_mensaje_get = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_id_mensaje_catch = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_mensajesLocalized = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_id_entrenadores = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_entrenador = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_hay_pokemones = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_especies_requeridas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_especies_que_llegaron = PTHREAD_MUTEX_INITIALIZER;

int cantidadElementosArray(char** array)
{
	int i = 0;
	while(array[i])
	{
		i++;
	}
	return i;
}

void liberarArray(char** array)
{
	for(int i = 0; i < cantidadElementosArray(array); i++)
		free(array[i]);
	free(array);
}

void ponerEntrenadoresEnLista() {

	inicializarListasDeEstados();

	entrenadores = list_create(); //Creamos la lista de entrenadores

	char** coordenadasEntrenadores = config_get_array_value(config, "POSICIONES_ENTRENADORES");

	char** pokemonesDeEntrenadores = config_get_array_value(config, "POKEMON_ENTRENADORES");

	char** pokemonesObjetivoDeEntrenadores = config_get_array_value(config, "OBJETIVOS_ENTRENADORES");

	int i = 0, j = 0;

	t_list* listaDePokemonesDeEntrenadores = organizarPokemones(pokemonesDeEntrenadores);
	t_list* listaDePokemonesObjetivoDeEntrenadores = organizarPokemones(pokemonesObjetivoDeEntrenadores);

	while (coordenadasEntrenadores[i] != NULL) {

		t_coordenadas* coords = malloc(sizeof(t_coordenadas));

		coords->posX = atoi(&coordenadasEntrenadores[i][0]);
		coords->posY = atoi(&coordenadasEntrenadores[i][2]);

		t_list* pokemonesQueTiene = list_get(listaDePokemonesDeEntrenadores, j);
		t_list* pokemonesQueDesea = list_get(listaDePokemonesObjetivoDeEntrenadores, j);

		uint32_t id_entrenador = generar_id();

		t_entrenador* entrenador = crear_entrenador(id_entrenador, coords, pokemonesQueTiene, pokemonesQueDesea, NEW);

		pthread_mutex_lock(&mutex_entrenadores);
		list_add(entrenadores, entrenador);
		pthread_mutex_unlock(&mutex_entrenadores);

		pthread_mutex_lock(&mutex_listaNuevos);
		list_add(listaNuevos, entrenador);
		pthread_mutex_unlock(&mutex_listaNuevos);
		j++;
		i++;

	}

	pthread_mutex_lock(&mutex_entrenadores);
	int cantEntrenadores = list_size(entrenadores);
	sem_init(&sem_buscarEntrenadorMasCercano, 0, cantEntrenadores);
	pthread_mutex_unlock(&mutex_entrenadores);

	hacerObjetivoTeam(listaDePokemonesDeEntrenadores, listaDePokemonesObjetivoDeEntrenadores);

	pendientes = list_duplicate(objetivoTeam);

	list_destroy(listaDePokemonesDeEntrenadores);
	list_destroy(listaDePokemonesObjetivoDeEntrenadores);

	liberarArray(coordenadasEntrenadores);
	liberarArray(pokemonesDeEntrenadores);
	liberarArray(pokemonesObjetivoDeEntrenadores);
}

void crearHilosEntrenadores() {

	hilosEntrenadores = list_create();
	sem_entrenadores_ejecutar = list_create();

	pthread_mutex_lock(&mutex_entrenadores);
	int cantidadEntrenadores = list_size(entrenadores);
	pthread_t pthread_id[cantidadEntrenadores];

	for (int i = 0; i < cantidadEntrenadores; i++) {

		t_entrenador* entrenador = (t_entrenador*) list_get(entrenadores, i);

		sem_t* semaforoDelEntrenador = malloc(sizeof(sem_t));

		sem_init(semaforoDelEntrenador, 0, 0);

		list_add(sem_entrenadores_ejecutar, (void*) semaforoDelEntrenador);

		pthread_create(&pthread_id[i], NULL, (void*) ejecutarEntrenador, entrenador);

		pthread_detach(pthread_id[i]);

		list_add(hilosEntrenadores, &pthread_id[i]);
	}
	pthread_mutex_unlock(&mutex_entrenadores);

}

t_entrenador* crear_entrenador(uint32_t id_entrenador, t_coordenadas* coordenadas, t_list* pokemonesQuePosee, t_list* pokemonesQueQuiere, status_code estado) {
	t_entrenador* entrenador = malloc(sizeof(t_entrenador));

	if (pokemonesQueQuiere == NULL) {
		pokemonesQueQuiere = list_create();
	}

	uint32_t cantidad_pokemons;
	if (pokemonesQuePosee == NULL) {
		pokemonesQuePosee = list_create();
		cantidad_pokemons = 0;
	} else {
		cantidad_pokemons = list_size(pokemonesQuePosee);
	}

	entrenador->id_entrenador = id_entrenador;
	entrenador->coordenadas = coordenadas;
	entrenador->pokemonesQuePosee = pokemonesQuePosee;
	entrenador->pokemonesQueQuiere = pokemonesQueQuiere;
	entrenador->cantidad_pokemons = cantidad_pokemons;
	entrenador->estado = estado;
	entrenador->idMensajeCaught = 0;
	entrenador->puedeAtrapar = 0;
	entrenador->esLocalized = 0;
	entrenador->misCiclosDeCPU = 0;
	entrenador->quantumDisponible = quantum;
	entrenador->quantumIntercambio = 5;
	entrenador->estimacionInicial = estimacionInicial;
	entrenador->rafagaAnteriorReal = 0;

	return entrenador;
}

t_list* organizarPokemones(char** listaPokemones) { //tanto para pokemonesObjetivoDeEntrenadores como para pokemonesDeEntrenadores

	int j = 0, w = 0;

	t_list* listaDePokemonesDeEntrenadores = list_create();

	while (listaPokemones[j] != NULL) { //recorro los pokemones de cada entrenador separado por coma
		char pipe = '|';
		char**pokemonesDeUnEntrenador = string_split(listaPokemones[j], &pipe); //separo cada pokemon de un mismo entrenador separado por |

		t_list* listaDePokemones = list_create();

		while (pokemonesDeUnEntrenador[w] != NULL) { //recorro todos y voy creando cada pokemon

			t_nombrePokemon* pokemon = crear_pokemon(pokemonesDeUnEntrenador[w]);

			list_add(listaDePokemones, pokemon);

			w++;
		}

		list_add(listaDePokemonesDeEntrenadores, listaDePokemones);

		j++;
		w=0;
		free(pokemonesDeUnEntrenador);
	}

	return listaDePokemonesDeEntrenadores;
}

t_nombrePokemon* crear_pokemon(char* pokemon) {

	t_nombrePokemon* nuevoPokemon = malloc(sizeof(t_nombrePokemon));

	nuevoPokemon->nombre_lenght = strlen(pokemon);
	nuevoPokemon->nombre = pokemon;

	return nuevoPokemon;

}

uint32_t generar_id() {
	pthread_mutex_lock(&mutex_id_entrenadores);
	uint32_t id_generado = ID_ENTRENADORES++;
	pthread_mutex_unlock(&mutex_id_entrenadores);

	return id_generado;
}

void hacerObjetivoTeam(t_list* listaMini, t_list* listaGrande){

	aplanarDobleLista(listaGrande);
	aplanarDobleLista(listaMini);

	diferenciaYCargarLista(listaGrande, listaMini, objetivoTeam);

}

void aplanarDobleLista(t_list* lista){

	t_list* listaDuplicada = list_duplicate(lista);

	list_clean(lista);

	int tamanioListaSuprema = list_size(listaDuplicada);

		for(int b=0; b<tamanioListaSuprema ;b++){

			 int tamanioSubLista = list_size(list_get(listaDuplicada, b));

			 for(int a=0; a<tamanioSubLista; a++){

				 list_add(lista, list_get(list_get(listaDuplicada, b), a));
			 }
		}

	list_destroy(listaDuplicada);
}

void ejecutarEntrenador(t_entrenador* entrenador){

	while(1) {
		sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
		sem_wait(semaforoDelEntrenador);

		if(entrenador->puedeAtrapar){

			t_nombrePokemon* pokemonAtrapado = malloc(sizeof(t_nombrePokemon));
			pokemonAtrapado = entrenador->pokemonInstantaneo->pokemon;

			list_add(entrenador->pokemonesQuePosee, (void*) pokemonAtrapado);
			entrenador->cantidad_pokemons++;

			log_atrapo_al_pokemon(entrenador->id_entrenador,
					pokemonAtrapado->nombre,
					entrenador->pokemonInstantaneo->coordenadas->posX,
					entrenador->pokemonInstantaneo->coordenadas->posY);

			pthread_mutex_lock(&mutex_atrapados);
			list_add(atrapados,(void*) pokemonAtrapado);
			pthread_mutex_unlock(&mutex_atrapados);

			pthread_mutex_lock(&mutex_pendientes);
			sacarPokemonDe(pokemonAtrapado, pendientes);
			pthread_mutex_unlock(&mutex_pendientes);

			//free(entrenador->pokemonInstantaneo->coordenadas);
			//free(entrenador->pokemonInstantaneo); TODO chequear esto
			entrenador->pokemonInstantaneo = NULL;
			sem_post(&sem_esperarCaught);

		} else {
			if((entrenador->pokemonInstantaneo) != NULL) {

				moverAlEntrenadorHastaUnPokemon(entrenador->id_entrenador);

				if(llegoAlObjetivoPokemon(entrenador)){
					uint32_t id = enviarMensajeCatch(entrenador->pokemonInstantaneo);

					if(id==0){
						log_error_comunicacion_con_broker();

						t_nombrePokemon* pokemonAtrapado = malloc(sizeof(t_nombrePokemon));
						pokemonAtrapado = entrenador->pokemonInstantaneo->pokemon;

						list_add(entrenador->pokemonesQuePosee, (void*) pokemonAtrapado);
						entrenador->cantidad_pokemons++;

						log_atrapo_al_pokemon(entrenador->id_entrenador,
										pokemonAtrapado->nombre,
										entrenador->pokemonInstantaneo->coordenadas->posX,
										entrenador->pokemonInstantaneo->coordenadas->posY);


						pthread_mutex_lock(&mutex_atrapados);
						list_add(atrapados,(void*) pokemonAtrapado);
						pthread_mutex_unlock(&mutex_atrapados);

						pthread_mutex_lock(&mutex_pendientes);
						sacarPokemonDe(pokemonAtrapado, pendientes);
						pthread_mutex_unlock(&mutex_pendientes);

						//free(entrenador->pokemonInstantaneo->coordenadas);
						//free(entrenador->pokemonInstantaneo); TODO chequear esto
						entrenador->pokemonInstantaneo = NULL;
						entrenador->esLocalized = 0;
					}
					entrenador->idMensajeCaught = id;

					sem_post(&sem_esperarCaught);
				}

				sem_post(&sem_entrenadorMoviendose);

			} else if (list_get(entrenadorIntercambio,0) != NULL) {
				t_entrenador* elEntrenador =  list_get(entrenadorIntercambio,0);
				moverAlEntrenadorHastaOtroEntrenador(entrenador->id_entrenador, elEntrenador->id_entrenador);

				sem_post(&sem_entrenadorMoviendose);
			} else {
				t_entrenador* entrenadorParaIntercambiar = elegirConQuienIntercambiar(entrenador);
				if (entrenadorParaIntercambiar != NULL) {
					list_add(entrenadorConQuienIntercambiar, entrenadorParaIntercambiar);
					intercambiarPokemones(entrenador->id_entrenador, entrenadorParaIntercambiar->id_entrenador);
					sem_post(&sem_entrenadorMoviendose);
				}
			}
		}
	}
}

uint32_t enviarMensajeCatch(t_newPokemon* pokemon){

	t_catchPokemon_msg* estructuraPokemon = malloc(sizeof(t_catchPokemon_msg));

	estructuraPokemon->coordenadas = *(pokemon->coordenadas);
	estructuraPokemon->nombre_pokemon = *(pokemon->pokemon);
	int socket_cliente = crear_conexion(ipBroker, puertoBroker);

	uint32_t id = 0;

	if(socket_cliente<=0){
		return id;
	}

	int status = enviar_mensaje(CATCH_POKEMON, 0, 0, estructuraPokemon, socket_cliente);

	if(status>=0){
		id = esperarIdCatch(socket_cliente);
	}

	liberar_conexion(socket_cliente);

	free(estructuraPokemon);
	return id;
}

uint32_t esperarIdCatch(int socket_cliente){
	uint32_t* id_respuesta = malloc(sizeof(uint32_t));
	*id_respuesta = recibir_id(socket_cliente);

	pthread_mutex_lock(&mutex_id_mensaje_catch);
	list_add(id_mensajeCatch,(void*) id_respuesta);
	pthread_mutex_unlock(&mutex_id_mensaje_catch);

	return *id_respuesta;
}

void moverAlEntrenadorHastaUnPokemon(uint32_t idEntrenador){

	sleep(retardoCPU);

	t_entrenador* entrenador = list_get(entrenadores, idEntrenador);

	uint32_t posicionXEntrenador = entrenador->coordenadas->posX;
	uint32_t posicionYEntrenador = entrenador->coordenadas->posY;

	uint32_t posicionXPokemon = entrenador->pokemonInstantaneo->coordenadas->posX;
	uint32_t posicionYPokemon = entrenador->pokemonInstantaneo->coordenadas->posY;

	if (posicionXEntrenador != posicionXPokemon) {

		int diferenciaEnX = posicionXPokemon - posicionXEntrenador;
		if (diferenciaEnX > 0) {
			entrenador->coordenadas->posX = posicionXEntrenador + 1;
		} else if (diferenciaEnX < 0) {
			entrenador->coordenadas->posX = posicionXEntrenador - 1;
		}

	} else if (posicionYEntrenador != posicionYPokemon) {

		int diferenciaEnY = posicionYPokemon - posicionYEntrenador;
		if (diferenciaEnY > 0) {
			entrenador->coordenadas->posY = posicionYEntrenador + 1;
		} else if (diferenciaEnY < 0) {
			entrenador->coordenadas->posY = posicionYEntrenador - 1;
		}

	}

	log_movimiento_entrenador(idEntrenador, entrenador->coordenadas->posX, entrenador->coordenadas->posY);
	entrenador->misCiclosDeCPU++;

}

t_entrenador* entrenadorMasCercano(t_newPokemon* pokemon){
	t_entrenador* entrenadorTemporal;
	t_entrenador* entrenadorMasCercanoBlocked;

	int distanciaTemporal;
	int menorDistanciaBlocked = 1000;

	bool noTienePokemonInstantaneo(void* elemento){
		t_entrenador* bloqueado = (t_entrenador*) elemento;
		return bloqueado->pokemonInstantaneo == NULL;
	}

	pthread_mutex_lock(&mutex_listaBloqueadosEsperandoPokemones);
	t_list* entrenadores_bloqueados = list_filter(listaBloqueadosEsperandoPokemones, noTienePokemonInstantaneo);
	pthread_mutex_unlock(&mutex_listaBloqueadosEsperandoPokemones);

	if(!list_is_empty(entrenadores_bloqueados)){
		entrenadorMasCercanoBlocked = list_get(entrenadores_bloqueados, 0);
		menorDistanciaBlocked = distanciaA(entrenadorMasCercanoBlocked->coordenadas, pokemon->coordenadas);

		for(int i=0; i < entrenadores_bloqueados->elements_count; i++){

			if(menorDistanciaBlocked ==0){
				break;
			}

			entrenadorTemporal = list_get(entrenadores_bloqueados, i);
			distanciaTemporal = distanciaA(entrenadorTemporal->coordenadas, pokemon->coordenadas);

			if(distanciaTemporal < menorDistanciaBlocked){
				entrenadorMasCercanoBlocked = entrenadorTemporal;
				menorDistanciaBlocked = distanciaTemporal;
			}
		}
	}

	list_destroy(entrenadores_bloqueados);

	pthread_mutex_lock(&mutex_listaNuevos);
	t_list* entrenadores_new = list_duplicate(listaNuevos);
	pthread_mutex_unlock(&mutex_listaNuevos);

	t_entrenador* entrenadorMasCercanoNew;
	int menorDistanciaNew = 1000;

	if(!list_is_empty(entrenadores_new)){
		entrenadorMasCercanoNew = list_get(entrenadores_new, 0);
		menorDistanciaNew = distanciaA(entrenadorMasCercanoNew->coordenadas, pokemon->coordenadas);


		for(int i = 1; i < entrenadores_new->elements_count; i++){

			if(menorDistanciaNew == 0){
				break;
			}

			entrenadorTemporal = list_get(entrenadores_new, i);
			distanciaTemporal = distanciaA(entrenadorTemporal->coordenadas, pokemon->coordenadas);

			if(distanciaTemporal < menorDistanciaNew){
				entrenadorMasCercanoNew = entrenadorTemporal;
				menorDistanciaNew = distanciaTemporal;
			}

		}
	}

	list_destroy(entrenadores_new);

	if(menorDistanciaNew <= menorDistanciaBlocked){

		pthread_mutex_lock(&mutex_listaNuevos);
		sacarEntrenadorDeLista(entrenadorMasCercanoNew, listaNuevos);
		pthread_mutex_unlock(&mutex_listaNuevos);

		return entrenadorMasCercanoNew;

	} else{
		pthread_mutex_lock(&mutex_listaBloqueadosEsperandoPokemones);
		sacarEntrenadorDeLista(entrenadorMasCercanoBlocked, listaBloqueadosEsperandoPokemones);
		pthread_mutex_unlock(&mutex_listaBloqueadosEsperandoPokemones);

		return entrenadorMasCercanoBlocked;
	}

}

void buscarPokemonAppeared(t_newPokemon* pokemon){  //Busca al entrenador más cercano y pone a planificar (para que ejecute, es decir, para que busque al pokemon en cuestión)

	t_entrenador* entrenador = entrenadorMasCercano(pokemon);

	ponerEntrenadorEnReady(entrenador, pokemon);

}

void buscarPokemonLocalized(t_localizedPokemon_msg* mensajeLocalized, uint32_t idMensaje){  //Busca al entrenador más cercano y pone a planificar (para que ejecute, es decir, para que busque al pokemon en cuestión)

	int cantidadCoords = mensajeLocalized->cantidad_coordenadas;

	t_newPokemon* pokemonNuevo = malloc(sizeof(t_newPokemon));
	t_coordenadas* pkmCoordenadas = malloc(sizeof(t_coordenadas));
	pokemonNuevo->coordenadas = pkmCoordenadas;

	pokemonNuevo->pokemon = &(mensajeLocalized->nombre_pokemon);
	pokemonNuevo->coordenadas->posX = mensajeLocalized->coordenadas[0].posX;
	pokemonNuevo->coordenadas->posY = mensajeLocalized->coordenadas[0].posY;

	t_entrenador* entrenador = entrenadorMasCercano(pokemonNuevo);
	int distancia = distanciaA(entrenador->coordenadas, pokemonNuevo->coordenadas);

	int j=0;

	for(int i=1; i<cantidadCoords; i++){
		t_newPokemon* pokemonTemporal = malloc(sizeof(t_newPokemon));
		pokemonTemporal->pokemon = &(mensajeLocalized->nombre_pokemon);

		pokemonTemporal->coordenadas->posX = mensajeLocalized->coordenadas[i].posX;
		pokemonTemporal->coordenadas->posY = mensajeLocalized->coordenadas[i].posY;

		t_entrenador* entrenadorTemporal = entrenadorMasCercano(pokemonTemporal);
		int distanciaTemporal = distanciaA(entrenador->coordenadas, pokemonTemporal->coordenadas);

		if(distanciaTemporal < distancia){
			free(pokemonNuevo);
			entrenador = entrenadorTemporal;
			pokemonNuevo = pokemonTemporal;
			distancia = distanciaTemporal;
			j=i;
		}
	}

	entrenador->esLocalized = idMensaje;

	ponerEntrenadorEnReady(entrenador, pokemonNuevo);


	///t_coordenadas nuevasCoords[cantidadCoords-1];
	t_coordenadas* nuevasCoords = malloc((cantidadCoords-1) * sizeof(*nuevasCoords));

	int m=0;

	for(int i=0; i<cantidadCoords; i++){
			if(i!=j){
				nuevasCoords[m] = mensajeLocalized->coordenadas[i];
				m++;
			}
	}

	mensajeLocalized->cantidad_coordenadas = cantidadCoords-1;

	mensajeLocalized->coordenadas = nuevasCoords;

}

void ponerEntrenadorEnReady(t_entrenador* entrenador, t_newPokemon* pokemon){
	entrenador->estado = READY;

	pthread_mutex_lock(&mutex_listaReady);
	log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "es el mas cercano al pokemon que apareció", "READY");
	list_add(listaReady, entrenador);
	pthread_mutex_unlock(&mutex_listaReady);

	entrenador->pokemonInstantaneo = pokemon;
}

void moverAlEntrenadorHastaOtroEntrenador(uint32_t idEntrenador1, uint32_t idEntrenador2){

	sleep(retardoCPU);

	t_entrenador* entrenador1 = list_get(entrenadores, idEntrenador1);
	t_entrenador* entrenador2 = list_get(entrenadores, idEntrenador2);

	uint32_t posicionXEntrenador1 = entrenador1->coordenadas->posX;
	uint32_t posicionYEntrenador1 = entrenador1->coordenadas->posY;

	uint32_t posicionXEntrenador2 = entrenador2->coordenadas->posX;
	uint32_t posicionYEntrenador2 = entrenador2->coordenadas->posY;


	if (posicionXEntrenador1 != posicionXEntrenador2) {

		int diferenciaEnX = posicionXEntrenador2 - posicionXEntrenador1;
		if (diferenciaEnX > 0) {
			entrenador1->coordenadas->posX = posicionXEntrenador1 + 1;
		} else if (diferenciaEnX < 0) {
			entrenador1->coordenadas->posX = posicionXEntrenador1 - 1;
		}

	} else if (posicionYEntrenador1 != posicionYEntrenador2) {

		int diferenciaEnY = posicionYEntrenador2 - posicionYEntrenador1;
		if (diferenciaEnY > 0) {
			entrenador1->coordenadas->posY = posicionYEntrenador1 + 1;
		} else if (diferenciaEnY < 0) {
			entrenador1->coordenadas->posY = posicionYEntrenador1 - 1;
		}

	}

	log_movimiento_entrenador(idEntrenador1, entrenador1->coordenadas->posX, entrenador1->coordenadas->posY);

	entrenador1->misCiclosDeCPU++;

}

void intercambiarPokemones(uint32_t idEntrenador1, uint32_t idEntrenador2){

	t_entrenador* entrenador1 = list_get(entrenadores, idEntrenador1);
	t_entrenador* entrenador2 = list_get(entrenadores, idEntrenador2);

	if ((entrenador1->quantumIntercambio) <= (entrenador1->quantumDisponible) || (stringACodigoAlgoritmo(algoritmoPlanificacion) != RR)) {

		sleep((entrenador1->quantumIntercambio)*retardoCPU);
		intercambiarPokemonesEntre(entrenador1, entrenador2);

		entrenador1->misCiclosDeCPU += entrenador1->quantumIntercambio;

		log_intercambio_pokemones(idEntrenador1, idEntrenador2);

		entrenador1->quantumDisponible -= entrenador1->quantumIntercambio;

		if (stringACodigoAlgoritmo(algoritmoPlanificacion) == RR) {
			entrenador1->quantumIntercambio = 0;
		}

	} else {

		sleep((entrenador1->quantumDisponible)*retardoCPU);

		entrenador1->misCiclosDeCPU += entrenador1->quantumDisponible;

		entrenador1->quantumIntercambio -= entrenador1->quantumDisponible;
		entrenador1->quantumDisponible = 0;
	}
}

void dameTuPokemon(t_entrenador* entrenador1, t_entrenador* entrenador2){

	t_list* listaQuiere1 = list_duplicate(entrenador1->pokemonesQueQuiere);
	t_list* listaPosee1 = list_duplicate(entrenador1->pokemonesQuePosee);
	t_list* listaQuiere2 = list_duplicate(entrenador2->pokemonesQueQuiere);
	t_list* listaPosee2 = list_duplicate(entrenador2->pokemonesQuePosee);

	t_list* leFaltanParaObj1 = list_create();
	t_list* tienePeroNoQuiere2 = list_create();
	t_list* pokemonesDe2QueQuiere1 = list_create();

	diferenciaYCargarLista(listaQuiere1, listaPosee1, leFaltanParaObj1);
	diferenciaYCargarLista(listaPosee2, listaQuiere2, tienePeroNoQuiere2);
	t_list* tienePeroNoQuiere2AUX = list_duplicate(tienePeroNoQuiere2);
	diferenciaYCargarLista(tienePeroNoQuiere2, leFaltanParaObj1, pokemonesDe2QueQuiere1);

	t_list* listaParaCondicion;

	if(list_is_empty(pokemonesDe2QueQuiere1)) {
		listaParaCondicion = tienePeroNoQuiere2AUX;
	} else {
		listaParaCondicion = pokemonesDe2QueQuiere1;
	}

	bool condicion(void* elemento) {
		return sonIguales((t_nombrePokemon*) list_get(listaParaCondicion, 0), (t_nombrePokemon*) elemento);
	}

	void* elementoRemovido = list_remove_by_condition(entrenador2->pokemonesQuePosee, condicion);
	list_add(entrenador1->pokemonesQuePosee, elementoRemovido);


	list_destroy(listaQuiere1);
	list_destroy(listaPosee1);
	list_destroy(listaQuiere2);
	list_destroy(listaPosee2);
	list_destroy(leFaltanParaObj1);
	list_destroy(tienePeroNoQuiere2);
	list_destroy(tienePeroNoQuiere2AUX);
	list_destroy(pokemonesDe2QueQuiere1);
}

void sacarPokemonDe(t_nombrePokemon* pokemon, t_list* lista){
	int a = list_size(lista);
		for(int i=0; i<a ; i++){
			t_nombrePokemon* pokemonLista = list_get(lista, i);
			if(sonIguales(pokemon, pokemonLista)){
				list_remove(lista, i);
				break;
			}
		}
}
