#ifndef MENSAJES_H_
#define MENSAJES_H_

#include <stdint.h>

/****************************************
 **********CODIGOS DE OPERACION**********
 ****************************************/
typedef enum {
	NEW_POKEMON = 1,
	APPEARED_POKEMON = 2,
	CATCH_POKEMON = 3,
	CAUGHT_POKEMON = 4,
	GET_POKEMON = 5,
	LOCALIZED_POKEMON = 6,
	SUSCRIPCION = 7,
	ERROR_CODIGO = 9
} op_code;

/****************************************
 *********ESTRUCTURAS GENERALES**********
 ****************************************/

typedef struct
{
	uint32_t nombre_lenght;
	char* nombre;
} t_nombrePokemon;

typedef struct
{
	uint32_t posX;
	uint32_t posY;
} t_coordenadas;

typedef struct
{
	uint32_t ID;
	uint32_t ID_correlativo;
} t_ids_respuesta;


/****************************************
 *******ESTRUCTURAS DE LOS MENSAJES******
 ****************************************/

typedef struct
{
	t_nombrePokemon nombre_pokemon;
	t_coordenadas coordenadas;
	uint32_t cantidad_pokemons;
} t_newPokemon_msg;

typedef struct
{
	t_nombrePokemon nombre_pokemon;
	t_coordenadas coordenadas;
} t_appearedPokemon_msg;

typedef struct
{
	t_nombrePokemon nombre_pokemon;
	t_coordenadas coordenadas;
} t_catchPokemon_msg;

typedef struct
{
	uint32_t atrapado;
} t_caughtPokemon_msg;

typedef struct
{
	t_nombrePokemon nombre_pokemon;
} t_getPokemon_msg;

typedef struct
{
	t_nombrePokemon nombre_pokemon;
	uint32_t cantidad_coordenadas;
	t_coordenadas* coordenadas;
} t_localizedPokemon_msg;


typedef struct
{
	uint32_t id_proceso;
	op_code tipo_cola;
	uint32_t temporal; // 0: indeterminado 0 < tiempo hasta el timeout
} t_suscripcion_msg;

char* op_code_a_string(op_code code);

#endif /* MENSAJES_H_ */
