/*
 * planificador.c
 *
 *  Created on: 15 jun. 2020
 *      Author: utnso
 */

#include "planificador.h"

pthread_mutex_t mutex_atrapados = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_pendientes = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_objetivoTeam = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_entrenadores = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_listaNuevos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_listaReady = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_listaBloqueadosDeadlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_listaBloqueadosEsperandoMensaje = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_listaBloqueadosEsperandoPokemones = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_listaFinalizados = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_cantidadDeadlocks = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cantidadCambiosContexto = PTHREAD_MUTEX_INITIALIZER;

void planificarCaught() {

	int cantidadCaughts1 = 0;
	pthread_mutex_lock(&mutex_listaBloqueadosEsperandoMensaje);
	if(!(list_is_empty(listaBloqueadosEsperandoMensaje))){
		for(int i=0; i < list_size(listaBloqueadosEsperandoMensaje) ; i++){

			t_entrenador* entrenador = list_get(listaBloqueadosEsperandoMensaje, i);

			if(entrenador->puedeAtrapar){
				entrenador->estado = READY;
				log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "llegó un caught que le permite atrapar al pokemon", "READY");

				pthread_mutex_lock(&mutex_listaReady);
				list_add(listaReady, entrenador);
				pthread_mutex_unlock(&mutex_listaReady);

				list_remove(listaBloqueadosEsperandoMensaje, i);
				cantidadCaughts1++;
				i--;
			}

			if((entrenador->pokemonInstantaneo) == NULL){

				pthread_mutex_lock(&mutex_listaBloqueadosEsperandoPokemones);
				list_add(listaBloqueadosEsperandoPokemones, entrenador);
				pthread_mutex_unlock(&mutex_listaBloqueadosEsperandoPokemones);
				list_remove(listaBloqueadosEsperandoMensaje, i);
				i--;
			}
		}
	}
	pthread_mutex_unlock(&mutex_listaBloqueadosEsperandoMensaje);

	for (int i=0; i < cantidadCaughts1; i++) {
		planificarSegun();
	}
}

void planificarSegun() {

	switch (stringACodigoAlgoritmo(algoritmoPlanificacion)) {

		case FIFO:
			planificarSegunFifo();

			break;

		case RR:
			if (planificarSegunRR()){
				planificarSegun();
			}
			break;

		case SJFCD:
			if (planificarSegunSJFConDesalojo()) {
				planificarSegun();
			}
			break;

		case SJFSD:
			planificarSegunSJFSinDesalojo();
			break;

		case ERROR_CODIGO_ALGORITMO:
			break;

		default:
			break;

	}

}

void planificarSegunFifo() {
	int distancia;

	sem_wait(&sem_planificar);

	sem_init(&sem_esperarCaught, 0, 0);

	t_entrenador* entrenador = (t_entrenador*) list_remove(listaReady, 0);

	entrenador->estado = EXEC;

	pthread_mutex_lock(&mutex_cantidadCambiosContexto);
	cantidadCambiosDeContexto+=1;
	pthread_mutex_unlock(&mutex_cantidadCambiosContexto);

	log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "fue seleccionado para ejecutar", "EXEC");

	if(entrenador->puedeAtrapar) {
		// Esto es un caught 1
		sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
		sem_post(semaforoDelEntrenador);

		sem_wait(&sem_esperarCaught);
		verificarTieneTodoLoQueQuiere(entrenador);
		entrenador->puedeAtrapar = 0;

	} else {
		//Esto es un appeared o un localized
		sem_init(&sem_entrenadorMoviendose, 0, 0);
		sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
		sem_post(semaforoDelEntrenador);
		sem_wait(&sem_entrenadorMoviendose);
		distancia = distanciaA(entrenador->coordenadas, entrenador->pokemonInstantaneo != NULL ? entrenador->pokemonInstantaneo->coordenadas : 0);

		while (distancia != 0 && distancia != -1) {
			sem_post(semaforoDelEntrenador);
			sem_wait(&sem_entrenadorMoviendose);
			distancia = distanciaA(entrenador->coordenadas, entrenador->pokemonInstantaneo != NULL ? entrenador->pokemonInstantaneo->coordenadas : NULL);
		}
		sem_destroy(&sem_entrenadorMoviendose);

		sem_wait(&sem_esperarCaught);
		if(entrenador->idMensajeCaught){
			entrenador->estado = BLOCKED;
			log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "se queda esperando un caught", "BLOCKED");

			pthread_mutex_lock(&mutex_listaBloqueadosEsperandoMensaje);
			list_add(listaBloqueadosEsperandoMensaje, entrenador);
			pthread_mutex_unlock(&mutex_listaBloqueadosEsperandoMensaje);

		} else{
			verificarTieneTodoLoQueQuiere(entrenador);
		}
	}

	sem_destroy(&sem_esperarCaught);

	chequearDeadlock(FIFO);

	chequearSiEstaDisponible(entrenador);

	sem_post(&sem_planificar);
}

int planificarSegunRR(){

	int distancia;

	sem_wait(&sem_planificar);

	sem_init(&sem_esperarCaught, 0, 0);

	t_entrenador* entrenador = (t_entrenador*) list_remove(listaReady, 0);

	entrenador->estado = EXEC;

	pthread_mutex_lock(&mutex_cantidadCambiosContexto);
	cantidadCambiosDeContexto+=1;
	pthread_mutex_unlock(&mutex_cantidadCambiosContexto);

	log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "fue seleccionado para ejecutar", "EXEC");

	if(entrenador->puedeAtrapar) {
		// Esto es un caught 1
		sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
		sem_post(semaforoDelEntrenador);

		sem_wait(&sem_esperarCaught);
		verificarTieneTodoLoQueQuiere(entrenador);

		entrenador->puedeAtrapar = 0;

	} else {
		sem_init(&sem_entrenadorMoviendose, 0, 0);
		sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
		sem_post(semaforoDelEntrenador);

		sem_wait(&sem_entrenadorMoviendose);
		distancia = distanciaA(entrenador->coordenadas, entrenador->pokemonInstantaneo != NULL ? entrenador->pokemonInstantaneo->coordenadas : 0);

		entrenador->quantumDisponible -=1;

		while (((distancia != 0) && (entrenador->quantumDisponible)>0) && (distancia != -1)) {
			sem_post(semaforoDelEntrenador);
			sem_wait(&sem_entrenadorMoviendose);
			distancia = distanciaA(entrenador->coordenadas, entrenador->pokemonInstantaneo != NULL ? entrenador->pokemonInstantaneo->coordenadas : 0);
			entrenador->quantumDisponible -= 1;
		}

		sem_destroy(&sem_entrenadorMoviendose);

		if(((entrenador->quantumDisponible)==0) && ((entrenador->pokemonInstantaneo != NULL ? !llegoAlObjetivoPokemon(entrenador) : 0))){
			list_add(listaReady, entrenador);
			entrenador->estado = READY;
			log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "se le terminó el quantum", "READY");

			entrenador->quantumDisponible = quantum;
			sem_destroy(&sem_esperarCaught);
			sem_post(&sem_planificar);
			return 1;
		} else{
			sem_wait(&sem_esperarCaught);
			entrenador->quantumDisponible = quantum;
			if(entrenador->idMensajeCaught){
				entrenador->estado = BLOCKED;
				log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "se queda esperando un caught", "BLOCKED");

				pthread_mutex_lock(&mutex_listaBloqueadosEsperandoMensaje);
				list_add(listaBloqueadosEsperandoMensaje, entrenador);
				pthread_mutex_unlock(&mutex_listaBloqueadosEsperandoMensaje);

			} else{
				verificarTieneTodoLoQueQuiere(entrenador);
			}
		}
	}

	sem_destroy(&sem_esperarCaught);

	chequearDeadlock(RR);

	chequearSiEstaDisponible(entrenador);

	sem_post(&sem_planificar);

	return 0;
}

int planificarSegunSJFConDesalojo(){

	int distancia;

	sem_wait(&sem_planificar);

	ordenarListaPorEstimacion(listaReady);

	sem_init(&sem_esperarCaught, 0, 0);

	t_entrenador* entrenador = (t_entrenador*) list_remove(listaReady, 0);

	int tamanioReadyActual = list_size(listaReady);

	//Cambia los datos del entrenador para cuando calcule su prox rafaga
	double estimadoProxRafaga = alfa * (entrenador->rafagaAnteriorReal) + (1-alfa)*(entrenador->estimacionInicial);
	entrenador->estimacionInicial = estimadoProxRafaga;

	entrenador->estado = EXEC;

	pthread_mutex_lock(&mutex_cantidadCambiosContexto);
	cantidadCambiosDeContexto+=1;
	pthread_mutex_unlock(&mutex_cantidadCambiosContexto);

	log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "fue seleccionado para ejecutar", "EXEC");

	if(entrenador->puedeAtrapar) {
		// Esto es un caught 1
		sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
		sem_post(semaforoDelEntrenador);
		sem_wait(&sem_esperarCaught);
		verificarTieneTodoLoQueQuiere(entrenador);

		entrenador->puedeAtrapar = 0;

	} else {
		sem_init(&sem_entrenadorMoviendose, 0, 0);
		sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);

		//Cambia los datos del entrenador para cuando calcule su prox rafaga
		entrenador->rafagaAnteriorReal = distanciaA(entrenador->coordenadas, entrenador->pokemonInstantaneo->coordenadas);

		sem_post(semaforoDelEntrenador);
		sem_wait(&sem_entrenadorMoviendose);

		distancia = distanciaA(entrenador->coordenadas, entrenador->pokemonInstantaneo != NULL ? entrenador->pokemonInstantaneo->coordenadas : 0);

		while (distancia != 0 && distancia != -1) {

			if ((tamanioReadyActual < list_size(listaReady))) {
				int distanciaQueLeQueda = distanciaA(entrenador->coordenadas, entrenador->pokemonInstantaneo->coordenadas);
				entrenador->rafagaAnteriorReal = entrenador->rafagaAnteriorReal - distanciaQueLeQueda;
				entrenador->estado = READY;
				log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "replanificó", "READY");
				pthread_mutex_lock(&mutex_listaReady);
				list_add(listaReady, entrenador);
				pthread_mutex_unlock(&mutex_listaReady);
				sem_destroy(&sem_esperarCaught);
				sem_post(&sem_planificar);
				return 1;
			}

			sem_post(semaforoDelEntrenador);
			sem_wait(&sem_entrenadorMoviendose);
			distancia = distanciaA(entrenador->coordenadas, entrenador->pokemonInstantaneo != NULL ? entrenador->pokemonInstantaneo->coordenadas : NULL);
		}

		sem_destroy(&sem_entrenadorMoviendose);

		sem_wait(&sem_esperarCaught);
		if(entrenador->idMensajeCaught){
			entrenador->estado = BLOCKED;
			log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "se queda esperando un caught", "BLOCKED");

			pthread_mutex_lock(&mutex_listaBloqueadosEsperandoMensaje);
			list_add(listaBloqueadosEsperandoMensaje, entrenador);
			pthread_mutex_unlock(&mutex_listaBloqueadosEsperandoMensaje);

		} else{
			verificarTieneTodoLoQueQuiere(entrenador);
		}
	}

	sem_destroy(&sem_esperarCaught);

	chequearDeadlock(SJFCD);

	chequearSiEstaDisponible(entrenador);

	sem_post(&sem_planificar);

	return 0;
}

void planificarSegunSJFSinDesalojo(){

	int distancia;

	sem_wait(&sem_planificar);

	ordenarListaPorEstimacion(listaReady);

	sem_init(&sem_esperarCaught, 0, 0);

	t_entrenador* entrenador = (t_entrenador*) list_remove(listaReady, 0);

	//Cambia los datos del entrenador para cuando calcule su prox rafaga
	double estimadoProxRafaga = alfa * (entrenador->rafagaAnteriorReal) + (1-alfa)*(entrenador->estimacionInicial);
	entrenador->estimacionInicial = estimadoProxRafaga;

	entrenador->estado = EXEC;

	pthread_mutex_lock(&mutex_cantidadCambiosContexto);
	cantidadCambiosDeContexto+=1;
	pthread_mutex_unlock(&mutex_cantidadCambiosContexto);

	log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "fue seleccionado para ejecutar", "EXEC");

	if(entrenador->puedeAtrapar) {
		// Esto es un caught 1
		sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
		sem_post(semaforoDelEntrenador);

		sem_wait(&sem_esperarCaught);
		verificarTieneTodoLoQueQuiere(entrenador);

		entrenador->puedeAtrapar = 0;

	} else {
		sem_init(&sem_entrenadorMoviendose, 0, 0);
		sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);

		//Cambia los datos del entrenador para cuando calcule su prox rafaga
		entrenador->rafagaAnteriorReal = distanciaA(entrenador->coordenadas, entrenador->pokemonInstantaneo->coordenadas);

		sem_post(semaforoDelEntrenador);

		sem_wait(&sem_entrenadorMoviendose);
		distancia = distanciaA(entrenador->coordenadas, entrenador->pokemonInstantaneo != NULL ? entrenador->pokemonInstantaneo->coordenadas : 0);

		while (distancia != 0 && distancia != -1) {
			sem_post(semaforoDelEntrenador);
			sem_wait(&sem_entrenadorMoviendose);
			distancia = distanciaA(entrenador->coordenadas, entrenador->pokemonInstantaneo != NULL ? entrenador->pokemonInstantaneo->coordenadas : NULL);
		}

		sem_destroy(&sem_entrenadorMoviendose);

		sem_wait(&sem_esperarCaught);
		if(entrenador->idMensajeCaught){
			entrenador->estado = BLOCKED;
			log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "se queda esperando un caught", "BLOCKED");

			pthread_mutex_lock(&mutex_listaBloqueadosEsperandoMensaje);
			list_add(listaBloqueadosEsperandoMensaje, entrenador);
			pthread_mutex_unlock(&mutex_listaBloqueadosEsperandoMensaje);

		} else{
			verificarTieneTodoLoQueQuiere(entrenador);
		}
	}

	sem_destroy(&sem_esperarCaught);

	chequearDeadlock(SJFSD);

	chequearSiEstaDisponible(entrenador);

	sem_post(&sem_planificar);
}

void chequearSiEstaDisponible(t_entrenador* entrenador){

	bool estaEnLista(void* elemento) {
		t_entrenador* otroEntrenador = (t_entrenador*) elemento;
		return otroEntrenador->id_entrenador == entrenador->id_entrenador;
	}

	pthread_mutex_lock(&mutex_listaBloqueadosEsperandoMensaje);
	bool estaEnListaCatch = list_any_satisfy(listaBloqueadosEsperandoMensaje, estaEnLista);
	pthread_mutex_unlock(&mutex_listaBloqueadosEsperandoMensaje);

	pthread_mutex_lock(&mutex_listaBloqueadosDeadlock);
	bool estaEnListaDeadlock = list_any_satisfy(listaBloqueadosDeadlock, estaEnLista);
	pthread_mutex_unlock(&mutex_listaBloqueadosDeadlock);

	if (!estaEnListaCatch && !estaEnListaDeadlock && entrenador->estado != FINISHED) {
		sem_post(&sem_buscarEntrenadorMasCercano);
	}
}

void chequearDeadlock(int algoritmo) {

	int tamanioObjetivoTeam = list_size(objetivoTeam);

	pthread_mutex_lock(&mutex_atrapados);
	int tamanioAtrapados = list_size(atrapados);
	pthread_mutex_unlock(&mutex_atrapados);

	if(tamanioObjetivoTeam == tamanioAtrapados){ //o cumplio el objetivo o hay deadlock

		int tamanioEntrenadores = list_size(entrenadores);

		pthread_mutex_lock(&mutex_listaFinalizados);
		int tamanioFinalizados = list_size(listaFinalizados);
		pthread_mutex_unlock(&mutex_listaFinalizados);

		if(tamanioEntrenadores == tamanioFinalizados || list_size(listaBloqueadosDeadlock) == 1){
			finalizarTeam();
		} else {
			int distancia;
			int i = 0;
			t_entrenador* entrenadorBloqueadoParaIntercambio;

			pthread_mutex_lock(&mutex_listaBloqueadosDeadlock);

			log_inicio_algoritmo_deadlock();
			inicioAlgoritmoDeadlock = 1;

			switch (algoritmo) {

				case FIFO:

					entrenadorBloqueadoParaIntercambio = list_remove(listaBloqueadosDeadlock, 0);
					list_add(entrenadorIntercambio, entrenadorBloqueadoParaIntercambio);
					for (int i=0; i < list_size(listaBloqueadosDeadlock); i++) {
						t_entrenador* entrenador = list_get(listaBloqueadosDeadlock, i);
						entrenador->estado = READY;
						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "se ejecutó el algoritmo de detección de deadlocks", "READY");
					}

					for (int i=0; i < list_size(listaBloqueadosDeadlock); i++) {
						t_entrenador* entrenador = list_get(listaBloqueadosDeadlock, i);
						entrenador->estado = EXEC;

						pthread_mutex_lock(&mutex_cantidadCambiosContexto);
						cantidadCambiosDeContexto+=1;
						pthread_mutex_unlock(&mutex_cantidadCambiosContexto);

						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "va a la posición de un entrenador para intercambiar", "EXEC");

						sem_init(&sem_entrenadorMoviendose, 0, 0);
						sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
						sem_post(semaforoDelEntrenador);

						sem_wait(&sem_entrenadorMoviendose);
						distancia = distanciaA(entrenador->coordenadas, entrenadorBloqueadoParaIntercambio->coordenadas);

						while (distancia != 0 && distancia != -1) {
							sem_post(semaforoDelEntrenador);
							sem_wait(&sem_entrenadorMoviendose);
							distancia = distanciaA(entrenador->coordenadas, entrenadorBloqueadoParaIntercambio->coordenadas);
						}

						entrenador->estado = BLOCKED;
						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "llegó a la posición del entrenador para intercambiar", "BLOCKED");

						sem_destroy(&sem_entrenadorMoviendose);
					}
					list_add_in_index(listaBloqueadosDeadlock, 0, entrenadorBloqueadoParaIntercambio);
					list_remove(entrenadorIntercambio, 0);

					while (list_size(listaBloqueadosDeadlock) > 1 && list_size(listaBloqueadosDeadlock) > list_size(entrenadoresNoSeleccionables)) {

						t_entrenador* entrenador = list_get(listaBloqueadosDeadlock, i);
						entrenador->estado = EXEC;

						pthread_mutex_lock(&mutex_cantidadCambiosContexto);
						cantidadCambiosDeContexto+=1;
						pthread_mutex_unlock(&mutex_cantidadCambiosContexto);

						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "va a intercambiar pokemones con otro entrenador", "EXEC");

						sem_init(&sem_entrenadorMoviendose, 0, 0);
						sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
						sem_post(semaforoDelEntrenador);

						sem_wait(&sem_entrenadorMoviendose);

						t_entrenador* entrenadorParaIntercambiar = list_get(entrenadorConQuienIntercambiar, 0);
						verificarTieneTodoLoQueQuiere(entrenador);
						verificarTieneTodoLoQueQuiere(entrenadorParaIntercambiar);

						pthread_mutex_lock(&mutex_cantidadDeadlocks);
						if(entrenador->estado == BLOCKED){
							if(entrenadorParaIntercambiar->estado == BLOCKED){
								log_fin_algoritmo_deadlock("ambos entrenadores siguen en deadlock.\n");
							} else{
								cantidadDeadlocksResueltos+=1;
								log_fin_algoritmo_deadlock("el entrenador elegido sigue en deadlock, sin embargo el elegido para intercambiar finalizó.\n");
							}

						} else{
							cantidadDeadlocksResueltos+=1;
							if(entrenadorParaIntercambiar->estado == BLOCKED){
								log_fin_algoritmo_deadlock("el entrenador que ejecutó pasó a estado finalizado, sin embargo el entrenador con el que intercambió sigue en deadlock.\n");
							} else{
								cantidadDeadlocksResueltos+=1;
								log_fin_algoritmo_deadlock("ambos entrenadores finalizaron, consiguiendo los pokemones que desean.\n");
							}
						}
						pthread_mutex_unlock(&mutex_cantidadDeadlocks);

						list_remove(entrenadorConQuienIntercambiar, 0);

						i++;
						if (i > list_size(listaBloqueadosDeadlock) - 1) {
							i = 0;
						}
					}

					break;

				case RR:

					entrenadorBloqueadoParaIntercambio = list_remove(listaBloqueadosDeadlock, 0);
					entrenadorBloqueadoParaIntercambio->quantumDisponible = quantum;
					list_add(entrenadorIntercambio, entrenadorBloqueadoParaIntercambio);

					for (int i=0; i < list_size(listaBloqueadosDeadlock); i++) {
						t_entrenador* entrenador = list_get(listaBloqueadosDeadlock, i);
						entrenador->estado = READY;
						entrenador->quantumDisponible = quantum;
						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "se ejecutó el algoritmo de detección de deadlocks", "READY");
					}

					t_list* listaDeadlockDuplicada = list_duplicate(listaBloqueadosDeadlock);

					while(!list_is_empty(listaDeadlockDuplicada)) {

						t_entrenador* entrenador = list_remove(listaDeadlockDuplicada, 0);
						entrenador->estado = EXEC;

						pthread_mutex_lock(&mutex_cantidadCambiosContexto);
						cantidadCambiosDeContexto+=1;
						pthread_mutex_unlock(&mutex_cantidadCambiosContexto);

						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "va a la posición de un entrenador para intercambiar", "EXEC");

						sem_init(&sem_entrenadorMoviendose, 0, 0);
						sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
						sem_post(semaforoDelEntrenador);
						sem_wait(&sem_entrenadorMoviendose);
						distancia = distanciaA(entrenador->coordenadas, entrenadorBloqueadoParaIntercambio->coordenadas);

						entrenador->quantumDisponible-=1;

						while (((distancia != 0) && (entrenador->quantumDisponible)>0) && (distancia != -1)) {
							sem_post(semaforoDelEntrenador);
							sem_wait(&sem_entrenadorMoviendose);
							distancia = distanciaA(entrenador->coordenadas, entrenadorBloqueadoParaIntercambio->coordenadas);

							entrenador->quantumDisponible -= 1;
						}

						if(((entrenador->quantumDisponible)==0) && (distancia!=0)){

							entrenador->estado = READY;
							log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "se le termino el quantum, no llego a la posicion del entrenador", "READY");

							entrenador->quantumDisponible = quantum;

							list_add(listaDeadlockDuplicada, entrenador);

						} else{
							entrenador->estado = BLOCKED;
							log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "llegó a la posición del entrenador para intercambiar", "BLOCKED");
							entrenador->quantumDisponible = quantum;
						}

						sem_destroy(&sem_entrenadorMoviendose);
					}

					list_destroy(listaDeadlockDuplicada);
					list_add_in_index(listaBloqueadosDeadlock, 0, entrenadorBloqueadoParaIntercambio);
					list_remove(entrenadorIntercambio, 0);

					while (list_size(listaBloqueadosDeadlock) > 1 && list_size(listaBloqueadosDeadlock) > list_size(entrenadoresNoSeleccionables)) {

						t_entrenador* entrenador = list_get(listaBloqueadosDeadlock, i);
						entrenador->estado = EXEC;

						pthread_mutex_lock(&mutex_cantidadCambiosContexto);
						cantidadCambiosDeContexto+=1;
						pthread_mutex_unlock(&mutex_cantidadCambiosContexto);

						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "va a intercambiar pokemones con otro entrenador", "EXEC");

						sem_init(&sem_entrenadorMoviendose, 0, 0);
						sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
						sem_post(semaforoDelEntrenador);
						sem_wait(&sem_entrenadorMoviendose);

						if(entrenador->quantumIntercambio){
							entrenador->estado = READY;
							entrenador->quantumDisponible = quantum;
							log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "se le terminó el quantum y no llegó a intercambiar", "READY");
						} else{
							t_entrenador* entrenadorParaIntercambiar = list_get(entrenadorConQuienIntercambiar, 0);
							verificarTieneTodoLoQueQuiere(entrenador);
							verificarTieneTodoLoQueQuiere(entrenadorParaIntercambiar);

							pthread_mutex_lock(&mutex_cantidadDeadlocks);
							if(entrenador->estado == BLOCKED){
								if(entrenadorParaIntercambiar->estado == BLOCKED){
									log_fin_algoritmo_deadlock("ambos entrenadores siguen en deadlock.\n");
								} else{
									cantidadDeadlocksResueltos+=1;
									log_fin_algoritmo_deadlock("el entrenador elegido sigue en deadlock, sin embargo el elegido para intercambiar finalizó.\n");
								}

							} else{
								cantidadDeadlocksResueltos+=1;
								if(entrenadorParaIntercambiar->estado == BLOCKED){
									i=-1;
									log_fin_algoritmo_deadlock("el entrenador que ejecutó pasó a estado finalizado, sin embargo el entrenador con el que intercambió sigue en deadlock.\n");
								} else{
									cantidadDeadlocksResueltos+=1;
									log_fin_algoritmo_deadlock("ambos entrenadores finalizaron, consiguiendo los pokemones que desean.\n");
								}
							}
							pthread_mutex_unlock(&mutex_cantidadDeadlocks);
						}

						list_remove(entrenadorConQuienIntercambiar, 0);

						i++;
						if (i > list_size(listaBloqueadosDeadlock) - 1) {
							i = 0;
						}

					}
					break;

				case SJFCD:

					ordenarListaPorEstimacion(listaBloqueadosDeadlock);
					entrenadorBloqueadoParaIntercambio = list_remove(listaBloqueadosDeadlock, 0);
					list_add(entrenadorIntercambio, entrenadorBloqueadoParaIntercambio);
					for (int i=0; i < list_size(listaBloqueadosDeadlock); i++) {
						t_entrenador* entrenador = list_get(listaBloqueadosDeadlock, i);
						entrenador->estado = READY;
						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "se ejecutó el algoritmo de detección de deadlocks", "READY");
					}

					for (int i=0; i < list_size(listaBloqueadosDeadlock); i++) {
						t_entrenador* entrenador = list_get(listaBloqueadosDeadlock, i);
						entrenador->estado = EXEC;

						pthread_mutex_lock(&mutex_cantidadCambiosContexto);
						cantidadCambiosDeContexto+=1;
						pthread_mutex_unlock(&mutex_cantidadCambiosContexto);

						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "va a la posición de un entrenador para intercambiar", "EXEC");

						//Cambia los datos del entrenador para cuando calcule su prox rafaga
						double estimadoProxRafaga = alfa * (entrenador->rafagaAnteriorReal) + (1-alfa)*(entrenador->estimacionInicial);
						entrenador->estimacionInicial = estimadoProxRafaga;
						entrenador->rafagaAnteriorReal = distanciaA(entrenador->coordenadas, entrenadorBloqueadoParaIntercambio->coordenadas);

						sem_init(&sem_entrenadorMoviendose, 0, 0);
						sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
						sem_post(semaforoDelEntrenador);

						sem_wait(&sem_entrenadorMoviendose);
						distancia = distanciaA(entrenador->coordenadas, entrenadorBloqueadoParaIntercambio->coordenadas);

						while (distancia != 0 && distancia != -1) {
							sem_post(semaforoDelEntrenador);
							sem_wait(&sem_entrenadorMoviendose);
							distancia = distanciaA(entrenador->coordenadas, entrenadorBloqueadoParaIntercambio->coordenadas);
						}

						entrenador->estado = BLOCKED;
						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "llegó a la posición del entrenador para intercambiar", "BLOCKED");

						sem_destroy(&sem_entrenadorMoviendose);
					}
					list_add_in_index(listaBloqueadosDeadlock, 0, entrenadorBloqueadoParaIntercambio);
					list_remove(entrenadorIntercambio, 0);

					ordenarListaPorEstimacion(listaBloqueadosDeadlock);
					while (list_size(listaBloqueadosDeadlock) > 1 && list_size(listaBloqueadosDeadlock) > list_size(entrenadoresNoSeleccionables)) {

						t_entrenador* entrenador = list_get(listaBloqueadosDeadlock, i);
						entrenador->estado = EXEC;

						pthread_mutex_lock(&mutex_cantidadCambiosContexto);
						cantidadCambiosDeContexto+=1;
						pthread_mutex_unlock(&mutex_cantidadCambiosContexto);

						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "va a intercambiar pokemones con otro entrenador", "EXEC");

						//Cambia los datos del entrenador para cuando calcule su prox rafaga
						double estimadoProxRafaga = alfa * (entrenador->rafagaAnteriorReal) + (1-alfa)*(entrenador->estimacionInicial);
						entrenador->estimacionInicial = estimadoProxRafaga;
						entrenador->rafagaAnteriorReal = 5;

						sem_init(&sem_entrenadorMoviendose, 0, 0);
						sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
						sem_post(semaforoDelEntrenador);
						sem_wait(&sem_entrenadorMoviendose);

						t_entrenador* entrenadorParaIntercambiar = list_get(entrenadorConQuienIntercambiar, 0);
						verificarTieneTodoLoQueQuiere(entrenador);
						verificarTieneTodoLoQueQuiere(entrenadorParaIntercambiar);

						pthread_mutex_lock(&mutex_cantidadDeadlocks);
						if(entrenador->estado == BLOCKED){
							if(entrenadorParaIntercambiar->estado == BLOCKED){
								log_fin_algoritmo_deadlock("ambos entrenadores siguen en deadlock.\n");
							} else{
								cantidadDeadlocksResueltos+=1;
								log_fin_algoritmo_deadlock("el entrenador elegido sigue en deadlock, sin embargo el elegido para intercambiar finalizó.\n");
							}

						} else{
							cantidadDeadlocksResueltos+=1;
							if(entrenadorParaIntercambiar->estado == BLOCKED){
								log_fin_algoritmo_deadlock("el entrenador que ejecutó pasó a estado finalizado, sin embargo el entrenador con el que intercambió sigue en deadlock.\n");
							} else{
								cantidadDeadlocksResueltos+=1;
								log_fin_algoritmo_deadlock("ambos entrenadores finalizaron, consiguiendo los pokemones que desean.\n");
							}
						}
						pthread_mutex_unlock(&mutex_cantidadDeadlocks);

						list_remove(entrenadorConQuienIntercambiar, 0);

						i++;
						if (i > list_size(listaBloqueadosDeadlock) - 1) {
							i = 0;
						}
					}

					break;
				case SJFSD:

					ordenarListaPorEstimacion(listaBloqueadosDeadlock);
					entrenadorBloqueadoParaIntercambio = list_remove(listaBloqueadosDeadlock, 0);
					list_add(entrenadorIntercambio, entrenadorBloqueadoParaIntercambio);
					for (int i=0; i < list_size(listaBloqueadosDeadlock); i++) {
						t_entrenador* entrenador = list_get(listaBloqueadosDeadlock, i);
						entrenador->estado = READY;
						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "se ejecutó el algoritmo de detección de deadlocks", "READY");
					}

					for (int i=0; i < list_size(listaBloqueadosDeadlock); i++) {
						t_entrenador* entrenador = list_get(listaBloqueadosDeadlock, i);
						entrenador->estado = EXEC;

						pthread_mutex_lock(&mutex_cantidadCambiosContexto);
						cantidadCambiosDeContexto+=1;
						pthread_mutex_unlock(&mutex_cantidadCambiosContexto);

						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "va a la posición de un entrenador para intercambiar", "EXEC");

						//Cambia los datos del entrenador para cuando calcule su prox rafaga
						double estimadoProxRafaga = alfa * (entrenador->rafagaAnteriorReal) + (1-alfa)*(entrenador->estimacionInicial);
						entrenador->estimacionInicial = estimadoProxRafaga;
						entrenador->rafagaAnteriorReal = distanciaA(entrenador->coordenadas, entrenadorBloqueadoParaIntercambio->coordenadas);

						sem_init(&sem_entrenadorMoviendose, 0, 0);
						sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
						sem_post(semaforoDelEntrenador);

						sem_wait(&sem_entrenadorMoviendose);
						distancia = distanciaA(entrenador->coordenadas, entrenadorBloqueadoParaIntercambio->coordenadas);

						while (distancia != 0 && distancia != -1) {
							sem_post(semaforoDelEntrenador);
							sem_wait(&sem_entrenadorMoviendose);
							distancia = distanciaA(entrenador->coordenadas, entrenadorBloqueadoParaIntercambio->coordenadas);
						}

						entrenador->estado = BLOCKED;
						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "llegó a la posición del entrenador para intercambiar", "BLOCKED");

						sem_destroy(&sem_entrenadorMoviendose);
					}
					list_add_in_index(listaBloqueadosDeadlock, 0, entrenadorBloqueadoParaIntercambio);
					list_remove(entrenadorIntercambio, 0);

					ordenarListaPorEstimacion(listaBloqueadosDeadlock);
					while (list_size(listaBloqueadosDeadlock) > 1 && list_size(listaBloqueadosDeadlock) > list_size(entrenadoresNoSeleccionables)) {

						t_entrenador* entrenador = list_get(listaBloqueadosDeadlock, i);
						entrenador->estado = EXEC;

						pthread_mutex_lock(&mutex_cantidadCambiosContexto);
						cantidadCambiosDeContexto+=1;
						pthread_mutex_unlock(&mutex_cantidadCambiosContexto);

						log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "va a intercambiar pokemones con otro entrenador", "EXEC");

						//Cambia los datos del entrenador para cuando calcule su prox rafaga
						double estimadoProxRafaga = alfa * (entrenador->rafagaAnteriorReal) + (1-alfa)*(entrenador->estimacionInicial);
						entrenador->estimacionInicial = estimadoProxRafaga;
						entrenador->rafagaAnteriorReal = 5;

						sem_init(&sem_entrenadorMoviendose, 0, 0);
						sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
						sem_post(semaforoDelEntrenador);
						sem_wait(&sem_entrenadorMoviendose);

						t_entrenador* entrenadorParaIntercambiar = list_get(entrenadorConQuienIntercambiar, 0);
						verificarTieneTodoLoQueQuiere(entrenador);
						verificarTieneTodoLoQueQuiere(entrenadorParaIntercambiar);

						pthread_mutex_lock(&mutex_cantidadDeadlocks);
						if(entrenador->estado == BLOCKED){
							if(entrenadorParaIntercambiar->estado == BLOCKED){
								log_fin_algoritmo_deadlock("ambos entrenadores siguen en deadlock.\n");
							} else{
								cantidadDeadlocksResueltos+=1;
								log_fin_algoritmo_deadlock("el entrenador elegido sigue en deadlock, sin embargo el elegido para intercambiar finalizó.\n");
							}

						} else{
							cantidadDeadlocksResueltos+=1;
							if(entrenadorParaIntercambiar->estado == BLOCKED){
								log_fin_algoritmo_deadlock("el entrenador que ejecutó pasó a estado finalizado, sin embargo el entrenador con el que intercambió sigue en deadlock.\n");
							} else{
								cantidadDeadlocksResueltos+=1;
								log_fin_algoritmo_deadlock("ambos entrenadores finalizaron, consiguiendo los pokemones que desean.\n");
							}
						}
						pthread_mutex_unlock(&mutex_cantidadDeadlocks);

						list_remove(entrenadorConQuienIntercambiar, 0);

						i++;
						if (i > list_size(listaBloqueadosDeadlock) - 1) {
							i = 0;
						}
					}

					break;

				case ERROR_CODIGO_ALGORITMO:

					puts("Se recibio mal el codigo\n");
					break;

				default:

					puts("Error desconocido\n");

					break;

				}
			pthread_mutex_unlock(&mutex_listaBloqueadosDeadlock);

			int cantidadEntrenadores = list_size(entrenadores);

			pthread_mutex_lock(&mutex_listaFinalizados);
			int cantidadFinalizados = list_size(listaFinalizados);
			pthread_mutex_unlock(&mutex_listaFinalizados);

			if(cantidadEntrenadores == cantidadFinalizados || list_size(listaBloqueadosDeadlock) == 1) {
				finalizarTeam();
			}

		}
	}
}


void ordenarListaPorEstimacion(t_list* list) {

	bool ordenarMenorCicloDeCPU(void* elemento1, void* elemento2){
		t_entrenador* entrenador1 = (t_entrenador*) elemento1;
		t_entrenador* entrenador2 = (t_entrenador*) elemento2;

		double estimadoProxRafaga1 = alfa * (entrenador1->rafagaAnteriorReal) + (1-alfa)*(entrenador1->estimacionInicial);
		double estimadoProxRafaga2 = alfa * (entrenador2->rafagaAnteriorReal) + (1-alfa)*(entrenador2->estimacionInicial);

		return estimadoProxRafaga1 <= estimadoProxRafaga2;
	}
	list_sort(list, ordenarMenorCicloDeCPU);
}

int distanciaA(t_coordenadas* desde, t_coordenadas* hasta){

	if (desde == NULL || hasta == NULL) {
		return -1;
	}

	int distanciaX = abs(desde->posX - hasta->posX);
	int distanciaY = abs(desde->posY - hasta->posY);

	return distanciaX + distanciaY;
}

int tieneTodoLoQueQuiere(t_entrenador* entrenador){

	t_list* listaTodoLoQueQuiere = list_duplicate(entrenador->pokemonesQueQuiere);
	t_list* listaTodoLoQuePosee = list_duplicate(entrenador->pokemonesQuePosee);
	t_list* diferencia = list_create();

	diferenciaYCargarLista(listaTodoLoQueQuiere, listaTodoLoQuePosee, diferencia);

	list_destroy(listaTodoLoQueQuiere);
	list_destroy(listaTodoLoQuePosee);

	int esVacia = list_is_empty(diferencia);

	list_destroy(diferencia);

	return esVacia;

}
//quiero yo / tiene el otro
void pokemonsQuePuedeDarle(t_list* listaA, t_list* listaB, t_list* listaACargar) {

	t_list* listaAAux = list_create();
	void cargarDiferenciaConB(void* elementoA) {
		t_nombrePokemon* pokemonA = (t_nombrePokemon*) elementoA;

		bool esElPkmBuscado(void* elementoB) {
			t_nombrePokemon* pokemonB = (t_nombrePokemon*) elementoB;
			return sonIguales(pokemonA, pokemonB);
		}

		if (list_find(listaAAux, esElPkmBuscado) == NULL) {
			t_list* listaAFiltrada = list_filter(listaA, esElPkmBuscado);
			t_list* listaBFiltrada =  list_filter(listaB, esElPkmBuscado);

			if (listaAFiltrada == NULL)
				listaAFiltrada = list_create();
			if (listaBFiltrada == NULL)
				listaBFiltrada = list_create();

			int sizeListaAFiltrada = list_size(listaAFiltrada);
			int sizeListaBFiltrada = list_size(listaBFiltrada);
			int diferencia = sizeListaAFiltrada - sizeListaBFiltrada;

			if (diferencia == 0 || diferencia < 0)
				for (int i=0; i < sizeListaAFiltrada; i++)
					list_add(listaACargar, pokemonA);

			if (diferencia > 0)
				for (int i=0; i < sizeListaBFiltrada; i++)
					list_add(listaACargar, pokemonA);

			list_destroy(listaAFiltrada);
			list_destroy(listaBFiltrada);
			list_add(listaAAux, pokemonA);
		}
	}

	list_iterate(listaA, cargarDiferenciaConB);
	list_destroy(listaAAux);
}

void diferenciasListasDeadlock(t_list* listaA, t_list* listaB, t_list* listaACargar) {

	t_list* listaAAux = list_create();
	void cargarDiferenciaConB(void* elementoA) {
		t_nombrePokemon* pokemonA = (t_nombrePokemon*) elementoA;

		bool esElPkmBuscado(void* elementoB) {
			t_nombrePokemon* pokemonB = (t_nombrePokemon*) elementoB;
			return sonIguales(pokemonA, pokemonB);
		}

		if (list_find(listaAAux, esElPkmBuscado) == NULL) {
			t_list* listaAFiltrada = list_filter(listaA, esElPkmBuscado);
			t_list* listaBFiltrada =  list_filter(listaB, esElPkmBuscado);

			if (listaAFiltrada == NULL)
				listaAFiltrada = list_create();
			if (listaBFiltrada == NULL)
				listaBFiltrada = list_create();

			int sizeListaAFiltrada = list_size(listaAFiltrada);
			int sizeListaBFiltrada = list_size(listaBFiltrada);
			int diferencia = sizeListaAFiltrada - sizeListaBFiltrada;
			for (int i=0; i < diferencia; i++)
				list_add(listaACargar, pokemonA);

			list_destroy(listaAFiltrada);
			list_destroy(listaBFiltrada);
			list_add(listaAAux, pokemonA);
		}
	}

	list_iterate(listaA, cargarDiferenciaConB);
	list_destroy(listaAAux);
}

void diferenciaYCargarLista(t_list* listaA, t_list* listaB, t_list* listaACargar) { 		//listaGrande A lista chica B

	int a = list_size(listaA);

	for(int i=0; i < a; i++){

		int b = list_size(listaB);
		int j=0;

		while((j < b) && (!sonIguales(list_get(listaB,j), list_get(listaA, i)))){
			j++;
		}

		if(j==b){
			list_add(listaACargar, list_get(listaA, i));
		}else{
			list_remove(listaB, j);
		}

	}
}

int sonIguales(t_nombrePokemon* pokemon1, t_nombrePokemon* pokemon2){
	return strcmp(pokemon1->nombre, pokemon2->nombre) == 0;
}

algoritmo_code stringACodigoAlgoritmo(const char* string) {
	for (int i = 0;
			i < sizeof(conversionAlgoritmo) / sizeof(conversionAlgoritmo[0]);
			i++) {
		if (!strcmp(string, conversionAlgoritmo[i].str))
			return conversionAlgoritmo[i].codigo_algoritmo;
	}
	return ERROR_CODIGO_ALGORITMO;
}

void inicializarListasDeEstados(){

	listaNuevos = list_create();
	listaReady = list_create();
	listaBloqueadosDeadlock= list_create();
	listaBloqueadosEsperandoMensaje= list_create();
	listaBloqueadosEsperandoPokemones = list_create();
	listaFinalizados = list_create();

}

void verificarTieneTodoLoQueQuiere(t_entrenador* entrenador){
	if(entrenador->cantidad_pokemons == list_size(entrenador->pokemonesQueQuiere)){
		if(tieneTodoLoQueQuiere(entrenador)){
			entrenador->estado = FINISHED;
			log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "tiene todo lo que quiere", "FINISHED"); //las wachas

			pthread_mutex_lock(&mutex_listaFinalizados);
			list_add(listaFinalizados, entrenador);
			pthread_mutex_unlock(&mutex_listaFinalizados);

			int indexEnDeadlock = getIndexEntrenadorEnLista(listaBloqueadosDeadlock, entrenador);
			if (indexEnDeadlock > -1) {
				list_remove(listaBloqueadosDeadlock, getIndexEntrenadorEnLista(listaBloqueadosDeadlock, entrenador));
			}
		} else{
			entrenador->estado = BLOCKED;
			log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "entró en deadlock", "BLOCKED");

			if (!inicioAlgoritmoDeadlock) {
				list_add(listaBloqueadosDeadlock, entrenador);
			}
		}
	}else{
		entrenador->estado = BLOCKED;
		log_entrenador_cambio_de_cola_planificacion(entrenador->id_entrenador, "no tiene todos los pokemones que quiere", "BLOCKED");

		pthread_mutex_lock(&mutex_listaBloqueadosEsperandoPokemones);
		list_add(listaBloqueadosEsperandoPokemones, entrenador);
		pthread_mutex_unlock(&mutex_listaBloqueadosEsperandoPokemones);
	}
}

int getIndexEntrenadorEnLista(t_list* lista, t_entrenador* entrenador) {
	if (lista->head == NULL)
		return -1;

	t_link_element *element = lista->head;
	t_entrenador* otroEntrenador = (t_entrenador*) (lista->head->data);

	int index = 0;
	while(element != NULL) {
		if (otroEntrenador->id_entrenador == entrenador->id_entrenador)
			return index;

		element = element->next;
		otroEntrenador = element == NULL ? NULL : element->data;
		index++;
	}

	return -1;
}

void sacarEntrenadorDeLista(t_entrenador* entrenador, t_list* lista){
	int a = list_size(lista);
	for(int i=0; i<a ; i++){
		t_entrenador* entrenadorDeLista = list_get(lista, i);
		if(entrenador->id_entrenador == entrenadorDeLista->id_entrenador){
			list_remove(lista, i);
			break;
		}
	}
}

void intercambiarPokemonesEntre(t_entrenador* entrenador1, t_entrenador* entrenador2) {
	t_list* listaQuiere1 = list_duplicate(entrenador1->pokemonesQueQuiere);
	t_list* listaPosee1 = list_duplicate(entrenador1->pokemonesQuePosee);
	t_list* leFaltanParaObj1 = list_create();
	diferenciasListasDeadlock(listaQuiere1, listaPosee1, leFaltanParaObj1);

//	for (int i = 0; i < list_size(leFaltanParaObj1); i++) {
//		t_nombrePokemon* pokemon = list_get(leFaltanParaObj1, i);
//		sleep(1);
//	}


	list_destroy(listaQuiere1);
	list_destroy(listaPosee1);
	listaQuiere1 = list_duplicate(entrenador1->pokemonesQueQuiere);
	listaPosee1 = list_duplicate(entrenador1->pokemonesQuePosee);
	t_list* leSobranAl1 = list_create();
	diferenciasListasDeadlock(listaPosee1, listaQuiere1, leSobranAl1);

//	for (int i = 0; i < list_size(leSobranAl1); i++) {
//		t_nombrePokemon* pokemon = list_get(leSobranAl1, i);
//		sleep(1);
//	}


	list_destroy(listaQuiere1);
	list_destroy(listaPosee1);

	t_list* listaQuiere2 = list_duplicate(entrenador2->pokemonesQueQuiere);
	t_list* listaPosee2 = list_duplicate(entrenador2->pokemonesQuePosee);
	t_list* leFaltanParaObj2 = list_create();
	diferenciasListasDeadlock(listaQuiere2, listaPosee2, leFaltanParaObj2);

//	for (int i = 0; i < list_size(leFaltanParaObj2); i++) {
//		t_nombrePokemon* pokemon = list_get(leFaltanParaObj2, i);
//		sleep(1);
//	}


	list_destroy(listaQuiere2);
	list_destroy(listaPosee2);
	listaQuiere2 = list_duplicate(entrenador2->pokemonesQueQuiere);
	listaPosee2 = list_duplicate(entrenador2->pokemonesQuePosee);
	t_list* leSobranAl2 = list_create();
	diferenciasListasDeadlock(listaPosee2, listaQuiere2, leSobranAl2);

//	for (int i = 0; i < list_size(leSobranAl2); i++) {
//		t_nombrePokemon* pokemon = list_get(leSobranAl2, i);
//		sleep(1);
//	}


	list_destroy(listaQuiere2);
	list_destroy(listaPosee2);

	t_list* pokemonesDe2QueQuiere1 = list_create();
	pokemonsQuePuedeDarle(leFaltanParaObj1, leSobranAl2, pokemonesDe2QueQuiere1);

//	for (int i = 0; i < list_size(pokemonesDe2QueQuiere1); i++) {
//		t_nombrePokemon* pokemon = list_get(pokemonesDe2QueQuiere1, i);
//		sleep(1);
//	}


	t_list* pokemonesDe1QueQuiere2 = list_create();
	pokemonsQuePuedeDarle(leFaltanParaObj2, leSobranAl1, pokemonesDe1QueQuiere2);


//	for (int i = 0; i < list_size(pokemonesDe1QueQuiere2); i++) {
//		t_nombrePokemon* pokemon = list_get(pokemonesDe1QueQuiere2, i);
//		sleep(1);
//	}

	t_nombrePokemon* pokemonBuscado;
	bool esElPkmBuscado(void* elementoB) {
		t_nombrePokemon* pokemonAComparar = (t_nombrePokemon*) elementoB;
		return sonIguales(pokemonBuscado, pokemonAComparar);
	}

	t_nombrePokemon* pokemonA;
	if (!list_is_empty(pokemonesDe2QueQuiere1)) {
		pokemonA = (t_nombrePokemon*) list_get(pokemonesDe2QueQuiere1, 0);
		list_add(entrenador1->pokemonesQuePosee, pokemonA);
	} else {
		pokemonA = (t_nombrePokemon*) list_get(leSobranAl2, 0);
		list_add(entrenador1->pokemonesQuePosee, pokemonA);
	}

	t_nombrePokemon* pokemonB;
	if (!list_is_empty(pokemonesDe1QueQuiere2)) {
		pokemonB = (t_nombrePokemon*) list_get(pokemonesDe1QueQuiere2, 0);
		list_add(entrenador2->pokemonesQuePosee, pokemonB);
	} else {
		pokemonB = (t_nombrePokemon*) list_get(leSobranAl1, 0);
		list_add(entrenador2->pokemonesQuePosee, pokemonB);
	}

	pokemonBuscado = pokemonB;
	list_remove_by_condition(entrenador1->pokemonesQuePosee, esElPkmBuscado);
	pokemonBuscado = pokemonA;
	list_remove_by_condition(entrenador2->pokemonesQuePosee, esElPkmBuscado);

	list_destroy(leFaltanParaObj1);
	list_destroy(leSobranAl1);
	list_destroy(leFaltanParaObj2);
	list_destroy(leSobranAl2);
	list_destroy(pokemonesDe2QueQuiere1);
	list_destroy(pokemonesDe1QueQuiere2);

}

t_entrenador* elegirConQuienIntercambiar(t_entrenador* entrenador) {


	t_list* listaQuiere1 = list_duplicate(entrenador->pokemonesQueQuiere);
	t_list* listaPosee1 = list_duplicate(entrenador->pokemonesQuePosee);

	t_list* leFaltanParaObj1 = list_create();


	diferenciasListasDeadlock(listaQuiere1, listaPosee1, leFaltanParaObj1);

//	for (int i = 0; i < list_size(leFaltanParaObj1); i++) {
//		t_nombrePokemon* pokemon = list_get(leFaltanParaObj1, i);
//		sleep(1);
//	}

	list_destroy(listaQuiere1);
	list_destroy(listaPosee1);

	t_list* sublistasPosiblesProveedoresDePokemon = list_create();
	t_list* posiblesProveedoresMenorPrioridad = list_create();

	int tamanioDeadlock = list_size(listaBloqueadosDeadlock);

	for(int a=0; a< tamanioDeadlock; a++) {

		t_entrenador* entrenador2 = list_get(listaBloqueadosDeadlock, a);

		if (entrenador2->id_entrenador == entrenador->id_entrenador) {
			continue;
		}

		t_list* pokemonesDe2QueQuiere1 = list_create();
		t_list* tienePeroNoQuiere2 = list_create();

		t_list* listaQuiere2 = list_duplicate(entrenador2->pokemonesQueQuiere);
		t_list* listaPosee2 = list_duplicate(entrenador2->pokemonesQuePosee);

		diferenciasListasDeadlock(listaPosee2, listaQuiere2, tienePeroNoQuiere2);

//		for (int i = 0; i < list_size(tienePeroNoQuiere2); i++) {
//			t_nombrePokemon* pokemon = list_get(tienePeroNoQuiere2, i);
//			sleep(1);
//		}

		t_list* tienePeroNoQuiere2AUX = list_duplicate(tienePeroNoQuiere2);

		list_destroy(listaQuiere2);
		list_destroy(listaPosee2);

		pokemonsQuePuedeDarle(leFaltanParaObj1, tienePeroNoQuiere2, pokemonesDe2QueQuiere1);

//		for (int i = 0; i < list_size(pokemonesDe2QueQuiere1); i++) {
//			t_nombrePokemon* pokemon = list_get(pokemonesDe2QueQuiere1, i);
//			sleep(1);
//		}

		if(!list_is_empty(pokemonesDe2QueQuiere1)){
			if(tengoAlgunPokemonQueQuiere2(entrenador, entrenador2)){
				list_destroy(sublistasPosiblesProveedoresDePokemon);
				list_destroy(posiblesProveedoresMenorPrioridad);
				list_destroy(pokemonesDe2QueQuiere1);
				list_destroy(tienePeroNoQuiere2);
				list_destroy(tienePeroNoQuiere2AUX);
				list_destroy(leFaltanParaObj1);
				return entrenador2;
			}
			list_add(sublistasPosiblesProveedoresDePokemon, entrenador2);
		} else if (!list_is_empty(tienePeroNoQuiere2AUX)) {
			list_add(posiblesProveedoresMenorPrioridad, entrenador2);
		}

		list_destroy(pokemonesDe2QueQuiere1);
		list_destroy(tienePeroNoQuiere2);
		list_destroy(tienePeroNoQuiere2AUX);
	}

	list_destroy(leFaltanParaObj1);

	t_entrenador* entrenadorProveedor = list_get(sublistasPosiblesProveedoresDePokemon, 0);

	if (entrenadorProveedor == NULL) {
		list_add(entrenadoresNoSeleccionables, entrenador);
		entrenadorProveedor = list_get(posiblesProveedoresMenorPrioridad, 0);
	}

	list_destroy(sublistasPosiblesProveedoresDePokemon);
	list_destroy(posiblesProveedoresMenorPrioridad);

	return entrenadorProveedor;
}

int tengoAlgunPokemonQueQuiere2(t_entrenador* entrenador1,t_entrenador* entrenador2){

	t_list* listaQuiere1 = list_duplicate(entrenador1->pokemonesQueQuiere);
	t_list* listaPosee1 = list_duplicate(entrenador1->pokemonesQuePosee);

	t_list* tienePeroNoQuiere1 = list_create();

	diferenciasListasDeadlock(listaPosee1, listaQuiere1, tienePeroNoQuiere1);

	t_list* listaQuiere2 = list_duplicate(entrenador2->pokemonesQueQuiere);
	t_list* listaPosee2 = list_duplicate(entrenador2->pokemonesQuePosee);

	t_list* leFaltanParaObj2 = list_create();

	diferenciasListasDeadlock(listaQuiere2, listaPosee2, leFaltanParaObj2);

	int tamanioFaltaParaObj2 = list_size(leFaltanParaObj2);
	int tamanioTienePeroNoQuiere1 = list_size(tienePeroNoQuiere1);

	list_destroy(listaQuiere1);
	list_destroy(listaPosee1);
	list_destroy(listaQuiere2);
	list_destroy(listaPosee2);

	for(int a=0; a< tamanioFaltaParaObj2; a++){

		for(int b=0; b<tamanioTienePeroNoQuiere1; b++){

			if(sonIguales(list_get(leFaltanParaObj2, a), list_get(tienePeroNoQuiere1, b))){
				list_destroy(leFaltanParaObj2);
				list_destroy(tienePeroNoQuiere1);
				return true;
			}
		}

	}

	list_destroy(leFaltanParaObj2);
	list_destroy(tienePeroNoQuiere1);

	return false;
}

int llegoAlObjetivoPokemon(t_entrenador* entrenador){

	uint32_t posicionXEntrenador = entrenador->coordenadas->posX;
	uint32_t posicionYEntrenador = entrenador->coordenadas->posY;

	uint32_t posicionXPokemon = entrenador->pokemonInstantaneo->coordenadas->posX;
	uint32_t posicionYPokemon = entrenador->pokemonInstantaneo->coordenadas->posY;

	return (posicionXEntrenador == posicionXPokemon) && (posicionYEntrenador == posicionYPokemon);
}

int llegoAlObjetivoEntrenador(t_entrenador* entrenador1, t_entrenador* entrenador2){

	uint32_t posicionXEntrenador1 = entrenador1->coordenadas->posX;
	uint32_t posicionYEntrenador1 = entrenador1->coordenadas->posY;

	uint32_t posicionXEntrenador2 = entrenador2->coordenadas->posX;
	uint32_t posicionYEntrenador2 = entrenador2->coordenadas->posY;

	return (posicionXEntrenador1 == posicionXEntrenador2) && (posicionYEntrenador1 == posicionYEntrenador2);
}

void finalizarTeam() {

	int ciclosCPUTotales = 0;
	char* cantidadCiclosCPUPorEntrenador =  string_new();

	for(int i=0; i< list_size(entrenadores);i++){

		t_entrenador* entrenador = list_get(entrenadores, i);
		ciclosCPUTotales += entrenador->misCiclosDeCPU;


		string_append_with_format(&cantidadCiclosCPUPorEntrenador, " el entrenador %d consumio: ", i);
		string_append_with_format(&cantidadCiclosCPUPorEntrenador, "%d", entrenador->misCiclosDeCPU);
		string_append(&cantidadCiclosCPUPorEntrenador, ";");

	}

	log_resultado_team("el team cumplió el objetivo", ciclosCPUTotales, cantidadCambiosDeContexto, cantidadCiclosCPUPorEntrenador, cantidadDeadlocksResueltos);

	// TODO FINALIZAR LISTAS Y LO QUE HAYA GLOBAL ACA
	void destruirSockets(void* elemento){
		int* socket = (int*) elemento;
		close(*socket);
	}

	list_destroy_and_destroy_elements(socketsALiberar, destruirSockets);

	void destruirEntrenadores(void* elemento){
		t_entrenador* entrenador = (t_entrenador*) elemento;
		free(entrenador->coordenadas);

		void liberarPokemones(void* elemento){
			t_nombrePokemon* pokemon = (t_nombrePokemon*) elemento;
			if(pokemon!=NULL){
				if(pokemon->nombre!=NULL){
					free(pokemon->nombre);
				}
				free(pokemon);
			}
		}

		list_destroy_and_destroy_elements(entrenador->pokemonesQuePosee, liberarPokemones);
		list_destroy_and_destroy_elements(entrenador->pokemonesQueQuiere, liberarPokemones);
		free(entrenador);
	}

	list_destroy_and_destroy_elements(entrenadores, destruirEntrenadores);

	config_destroy(config);

	exit(-1);
}

