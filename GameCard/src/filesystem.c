#include "filesystem.h"

char* directoriosACrear[] = {"Metadata", "Files", "Blocks"};
int BLOCKS;
int BLOCK_SIZE;
t_bitarray* bitarray;

void configuracionInicial(void)
{
	verificarDirectorio(PUNTO_MONTAJE);

	for(int i = 0; i < sizeof(directoriosACrear)/sizeof(char*); i++)
	{
		char* pathDirectorioACrear = obtenerRutaTotal(directoriosACrear[i]);
		verificarDirectorio(pathDirectorioACrear);
		verificarMetadata(pathDirectorioACrear);
		free(pathDirectorioACrear);
	}
}

/****************************************************
 **Funciones de verificación de archivos iniciales***
 ****************************************************/

void verificarMetadata(char* pathDirectorio)
{
	char* pathMetadata = string_from_format("%s/Metadata.bin", pathDirectorio);
	FILE* archivoMetadata = fopen(pathMetadata, "rb");

	if(!archivoMetadata)
	{
		archivoMetadata = fopen(pathMetadata, "wb");
		if(string_ends_with(pathDirectorio, "Metadata"))
			fprintf(archivoMetadata, "BLOCK_SIZE=64\nBLOCKS=1024\nMAGIC_NUMBER=TALL_GRASS");
		else
			fprintf(archivoMetadata, "DIRECTORY=Y");
	}
	fclose(archivoMetadata);

	if(string_ends_with(pathDirectorio, "Metadata"))
	{
		t_config* configMetadata = config_create(pathMetadata);
		BLOCKS = config_get_int_value(configMetadata, "BLOCKS");
		BLOCK_SIZE = config_get_int_value(configMetadata, "BLOCK_SIZE");
		config_destroy(configMetadata);
		verificarBitmap(pathDirectorio);
	}
	else if(string_ends_with(pathDirectorio, "Blocks")){
		verificarBloquesIniciales(pathDirectorio);
	}
	free(pathMetadata);
}

void verificarBitmap(char* pathDirectorio)
{
	char* pathBitmap = string_from_format("%s/Bitmap.bin", pathDirectorio);

	FILE* archivoActual = fopen(pathBitmap, "rb");
	if(!archivoActual)
		archivoActual = fopen(pathBitmap, "wb");
	fclose(archivoActual);

	truncate(pathBitmap, BLOCKS/8);
	int fileDescriptor = open(pathBitmap, O_RDWR);
	char* bitarrayFlujo = mmap(NULL, BLOCKS/8, PROT_WRITE | PROT_READ, MAP_SHARED, fileDescriptor, 0);
	bitarray = bitarray_create_with_mode(bitarrayFlujo, BLOCKS/8, LSB_FIRST);
	close(fileDescriptor);

	free(pathBitmap);
}

void verificarBloquesIniciales(char* pathDirectorio)
{
	FILE* archivoActual;
	for(int i = 1; i <= BLOCKS; i++)
	{
		char* pathBloqueActual = string_from_format("%s/%d.bin", pathDirectorio, i);

		archivoActual = fopen(pathBloqueActual, "rb");
		if(!archivoActual)
		{
			archivoActual = fopen(pathBloqueActual, "wb");
			truncate(pathBloqueActual, BLOCK_SIZE);
		}
		fclose(archivoActual);

		free(pathBloqueActual);
	}
}

/****************************************************
 **Funciones de verificación de archivos generales***
 ****************************************************/

void verificarDirectorio(char* pathDirectorio)
{
	DIR* directorioActual = opendir(pathDirectorio);

	if(!existePokemon(pathDirectorio))
			mkdir(pathDirectorio, 0777);

	closedir(directorioActual);
}

char* verificarPokemon(char* nombrePokemon)
{
	char* pathDirectorioFiles = obtenerRutaTotal("Files");
	char* pathDirectorioPokemon = string_from_format("%s/%s", pathDirectorioFiles, nombrePokemon);
	verificarDirectorio(pathDirectorioPokemon);

	char* pathMetadataPokemon = string_from_format("%s/Metadata.bin", pathDirectorioPokemon);
	verificarMetadataPokemon(pathMetadataPokemon);

	free(pathDirectorioPokemon);
	free(pathDirectorioFiles);
	return pathMetadataPokemon;
}

void verificarMetadataPokemon(char* pathMetadata)
{
	FILE* archivoActual = fopen(pathMetadata, "rb");
	if(!archivoActual)
	{
		archivoActual = fopen(pathMetadata, "wb");
		fprintf(archivoActual, "DIRECTORY=N\nBLOCKS=[]\nSIZE=0\nOPEN=N");
	}
	fclose(archivoActual);
}

char* verificarBloque(t_config* configMetadata)
{
	char** bloquesAsignados = config_get_array_value(configMetadata, "BLOCKS");
	int sizeBloquesPokemon = config_get_int_value(configMetadata, "SIZE");

	int cantidadBloques = cantidadElementosArray(bloquesAsignados);

	if(sePuedeEscribirElUltimoBloque(sizeBloquesPokemon))
	{
		char* rutaBloque = obtenerRutaBloque(atoi(bloquesAsignados[cantidadBloques-1]));
		liberarArray(bloquesAsignados);
		return rutaBloque;
	}
	else
	{
		liberarArray(bloquesAsignados);
		return asignarBloque(configMetadata);
	}
}

/*******************************************
 *************Funciones boolean*************
 ******************************************/

bool sePuedeEscribirElUltimoBloque(int sizeBloquesPokemon)
{
	return sizeBloquesPokemon%BLOCK_SIZE != 0;
}

bool existePokemon(char* nombrePokemon)
{
	char* pathDirectorioFiles = obtenerRutaTotal("Files");
	char* pathDirectorioPokemon = string_from_format("%s/%s", pathDirectorioFiles, nombrePokemon);

	DIR* directorioPokemon = opendir(pathDirectorioPokemon);

	bool existe = !(directorioPokemon == NULL);

	closedir(directorioPokemon);

	free(pathDirectorioFiles);
	free(pathDirectorioPokemon);
	return existe;
}

int existeCoordenada(char** lineasTotales, char* stringAEscribir)
{
	char** coordenadaABuscar = string_split(stringAEscribir, "=");
	for(int i = 0; lineasTotales[i]; i++)
	{
		char** coordenadaPelada = string_split(lineasTotales[i], "=");
		if(strcmp(coordenadaPelada[0], coordenadaABuscar[0]) == 0)
		{
			liberarArray(coordenadaPelada);
			liberarArray(coordenadaABuscar);
			return i;
		}
		liberarArray(coordenadaPelada);
	}
	liberarArray(coordenadaABuscar);
	return -1;
}

/*******************************************
 *******Funciones de índole general*********
 ******************************************/

char* armarArrayDeBloques(char** arrayOriginal, int numeroBloque)
{
	int i = 0;
	char* bloquesAsignadosComoArray = string_new();
	string_append(&bloquesAsignadosComoArray, "[");
	while(arrayOriginal[i])
	{
		string_append_with_format(&bloquesAsignadosComoArray, "%s,", arrayOriginal[i]);
		i++;
	}
	string_append_with_format(&bloquesAsignadosComoArray, "%d]", numeroBloque);
	return bloquesAsignadosComoArray;
}

int obtenerLugarEnBitmap()
{
	for(int i = 0; i < BLOCKS; i++)
	{
		if(!bitarray_test_bit(bitarray, i))
		{
			bitarray_set_bit(bitarray, i);
			msync((void*) bitarray->bitarray, BLOCKS/8, MS_SYNC);
			return i+1;
		}
	}
	return 0;
}

void obtenerCoordenadas(char** todasLasLineas, int* aDevolver)
{
	int j = 0;
	for(int i = 0; todasLasLineas[i]; i++)
	{
		char** lineaDividida = string_split(todasLasLineas[i], "=");
		char** coordenadasLinea = string_split(lineaDividida[0], "-");
		aDevolver[j] = atoi(coordenadasLinea[0]);
		aDevolver[j+1] = atoi(coordenadasLinea[1]);
		liberarArray(lineaDividida);
		liberarArray(coordenadasLinea);
		j+=2;
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


/*******************************************
 ******Funciones de obtención de rutas******
 ******************************************/

char* obtenerRutaTotal(char* path)
{
	char* pathTotal = string_from_format("%s/%s", PUNTO_MONTAJE, path);
	return pathTotal;
}

char* obtenerRutaBloque(int numeroBloque)
{
	char* pathDirectorioBlocks = obtenerRutaTotal("Blocks");
	char* pathBloque = string_from_format("%s/%d.bin", pathDirectorioBlocks, numeroBloque);
	free(pathDirectorioBlocks);
	return pathBloque;
}

/******************************************
 **Funciones de modificación de archivos***
 ******************************************/

t_config* abrirArchivo(char* pathArchivo)
{
	t_config* configArchivo = config_create(pathArchivo);
	char* estadoArchivo = config_get_string_value(configArchivo, "OPEN");
	while(strcmp(estadoArchivo, "Y") == 0)
	{
		sleep(config_get_int_value(configGeneral, "TIEMPO_DE_REINTENTO_OPERACION"));
	}
	config_set_value(configArchivo, "OPEN", "Y");
	config_save(configArchivo);
	return configArchivo;
}

void cerrarArchivo(t_config* configArchivo)
{
	sleep(config_get_int_value(configGeneral, "TIEMPO_RETARDO_OPERACION"));
	config_set_value(configArchivo, "OPEN", "N");
	config_save(configArchivo);
	config_destroy(configArchivo);
}

char* asignarBloque(t_config* configPokemon)
{
	int bloqueAAsignar = obtenerLugarEnBitmap();
	if(bloqueAAsignar > 0)
	{
		char** bloquesAsignados = config_get_array_value(configPokemon, "BLOCKS");
		char* bloquesAsignadosComoArray = armarArrayDeBloques(bloquesAsignados, bloqueAAsignar);

		config_set_value(configPokemon, "BLOCKS", bloquesAsignadosComoArray);
		config_save(configPokemon);

		free(bloquesAsignadosComoArray);
		liberarArray(bloquesAsignados);
		return obtenerRutaBloque(bloqueAAsignar);
	}
	else
		return NULL;
}

void escribirArchivoPokemon(char* stringAEscribir, t_config* configMetadataPokemon)
{
	char* pathBloqueAEscribir = verificarBloque(configMetadataPokemon);

	int bytesEscritos = escribirBloque(pathBloqueAEscribir, stringAEscribir);

	free(pathBloqueAEscribir);

	int sizeActual = config_get_int_value(configMetadataPokemon, "SIZE");
	char* bytesTotales = string_itoa(sizeActual+bytesEscritos);
	config_set_value(configMetadataPokemon, "SIZE", bytesTotales);
	config_save(configMetadataPokemon);
	free(bytesTotales);

	if(bytesEscritos < strlen(stringAEscribir))
	{
		char* pathNuevoBloqueAEscribir = verificarBloque(configMetadataPokemon);
		char* stringResto = string_substring_from(stringAEscribir, bytesEscritos);
		bytesEscritos += escribirBloque(pathNuevoBloqueAEscribir, stringResto);
		free(stringResto);
		free(pathNuevoBloqueAEscribir);
	}
	bytesTotales = string_itoa(sizeActual+bytesEscritos);
	config_set_value(configMetadataPokemon, "SIZE", bytesTotales);

	free(bytesTotales);
}

int escribirBloque(char* pathBloqueAEscribir, char* stringAEscribir)
{
	char* stringLeido = leerBloque(pathBloqueAEscribir);

	FILE* archivoBloque = fopen(pathBloqueAEscribir, "rb+");

	int bytesEscritos = 0;
	fseek(archivoBloque, strlen(stringLeido), SEEK_SET);
	if(strlen(stringLeido)+strlen(stringAEscribir) < BLOCK_SIZE)
		bytesEscritos = fwrite(stringAEscribir, sizeof(char), strlen(stringAEscribir), archivoBloque);
	else
		bytesEscritos = fwrite(stringAEscribir, sizeof(char), BLOCK_SIZE-strlen(stringLeido), archivoBloque);

	fclose(archivoBloque);
	free(stringLeido);
	return bytesEscritos;
}

int armarVectorCoordenadas(char** lineasTotales, int** aDevolver)
{
	int cantidadLineas = cantidadElementosArray(lineasTotales);

	int *coordenadasFinales = malloc(2*sizeof(int)*cantidadLineas);

	obtenerCoordenadas(lineasTotales, coordenadasFinales);

	*aDevolver = coordenadasFinales;

	return cantidadLineas;
}

char* leerBloque(char* pathBloque)
{
	FILE* archivoBloque = fopen(pathBloque, "rb");
	char* stringLeido = calloc(1, BLOCK_SIZE+1);
	fread(stringLeido, BLOCK_SIZE, 1, archivoBloque);
	fclose(archivoBloque);
	return stringLeido;
}

char* leerArchivoPokemon(t_config* configMetadataPokemon)
{
	char** bloquesAsignados = config_get_array_value(configMetadataPokemon, "BLOCKS");

	char* archivoPokemon = string_new();

	for(int i = 0; bloquesAsignados[i]; i++)
	{
		char* pathBloqueActual = obtenerRutaBloque(atoi(bloquesAsignados[i]));
		char* stringLeido = leerBloque(pathBloqueActual);
		string_append(&archivoPokemon, stringLeido);

		free(pathBloqueActual);
		free(stringLeido);
	}

	liberarArray(bloquesAsignados);
	return archivoPokemon;
}

char* modificarCoordenada(char* linea, int cantidadAAgregar)
{
	char** coordenadaActual = string_split(linea, "=");
	char* coordenadaNueva = string_from_format("%s=%d", coordenadaActual[0], atoi(coordenadaActual[1])+cantidadAAgregar);
	liberarArray(coordenadaActual);
	return coordenadaNueva;
}

char* armarArchivoPokemon(char** lineasTotales)
{
	char* archivoNuevoPokemon = string_new();
	for(int i = 0; lineasTotales[i]; i++)
	{
		if(strlen(lineasTotales[i]) != 0)
		{
			char* lineaAAgregar = string_from_format("%s\n", lineasTotales[i]);
			string_append(&archivoNuevoPokemon, lineaAAgregar);
			free(lineaAAgregar);
		}
	}
	return archivoNuevoPokemon;
}

void borrarBloques(t_config* configMetadataPokemon)
{
	config_set_value(configMetadataPokemon, "SIZE", "0");
	char** bloquesAsignados = config_get_array_value(configMetadataPokemon, "BLOCKS");
	config_set_value(configMetadataPokemon, "BLOCKS", "[]");

	for(int i = 0; i < cantidadElementosArray(bloquesAsignados); i++)
	{
		char* pathBloqueActual = obtenerRutaBloque(atoi(bloquesAsignados[i]));

		bitarray_clean_bit(bitarray, atoi(bloquesAsignados[i])-1);
		msync((void*) bitarray->bitarray, BLOCKS/8, MS_SYNC);

		FILE* bloqueActual = fopen(pathBloqueActual, "wb");
		fclose(bloqueActual);

		truncate(pathBloqueActual, BLOCK_SIZE);
		free(pathBloqueActual);
	}

	liberarArray(bloquesAsignados);
}

void rearmarArchivosPokemon(char** lineasTotales, t_config* configMetadataPokemon, int indiceCoordenada, int cantidadNueva)
{
	char* coordenadaModificada = modificarCoordenada(lineasTotales[indiceCoordenada], cantidadNueva);
	free(lineasTotales[indiceCoordenada]);
	lineasTotales[indiceCoordenada] = string_duplicate(coordenadaModificada);

	char* archivoNuevoPokemon = armarArchivoPokemon(lineasTotales);
	borrarBloques(configMetadataPokemon);
	escribirArchivoPokemon(archivoNuevoPokemon, configMetadataPokemon);

	free(archivoNuevoPokemon);
	free(coordenadaModificada);
}

char* borrarLinea(int indiceCoordenada, char** lineasTotales)
{
	free(lineasTotales[indiceCoordenada]);
	lineasTotales[indiceCoordenada] = string_new();
	return armarArchivoPokemon(lineasTotales);
}

/**************************************************
 ******Funciones de procesamiento de mensajes******
 **************************************************/

t_appearedPokemon_msg procesarNewPokemon(t_newPokemon_msg* estructuraNew)
{
	t_appearedPokemon_msg estructuraAppeared;
	char* nombrePokemon = arreglarNombrePokemon(estructuraNew->nombre_pokemon);

	char* pathMetadataPokemon = verificarPokemon(nombrePokemon);
	char* stringAEscribir = string_from_format("%d-%d=%d\n", estructuraNew->coordenadas.posX, estructuraNew->coordenadas.posY, estructuraNew->cantidad_pokemons);

	t_config* configMetadataPokemon = abrirArchivo(pathMetadataPokemon);
	char* archivoPokemonMapeado = leerArchivoPokemon(configMetadataPokemon);

	char** lineasTotales = string_split(archivoPokemonMapeado, "\n");

	int indiceCoordenada = existeCoordenada(lineasTotales, stringAEscribir);
	if(indiceCoordenada >= 0)
	{
		rearmarArchivosPokemon(lineasTotales, configMetadataPokemon, indiceCoordenada, estructuraNew->cantidad_pokemons);
	}
	else
	{
		escribirArchivoPokemon(stringAEscribir, configMetadataPokemon);
	}
	cerrarArchivo(configMetadataPokemon);
	free(archivoPokemonMapeado);
	liberarArray(lineasTotales);
	free(stringAEscribir);
	free(pathMetadataPokemon);
	free(nombrePokemon);

	estructuraAppeared.nombre_pokemon = estructuraNew->nombre_pokemon;
	estructuraAppeared.coordenadas = estructuraNew->coordenadas;
	return estructuraAppeared;
}

t_localizedPokemon_msg procesarGetPokemon(t_getPokemon_msg* estructuraGet)
{
	char* nombrePokemon = arreglarNombrePokemon(estructuraGet->nombre_pokemon);

	t_localizedPokemon_msg estructuraLocalized;

	if(!existePokemon(nombrePokemon))
	{
		estructuraLocalized.cantidad_coordenadas = 0;
		estructuraLocalized.nombre_pokemon = estructuraGet->nombre_pokemon;
		estructuraLocalized.coordenadas = malloc(sizeof(t_coordenadas));
		return estructuraLocalized;
	}

	char* pathMetadataPokemon = verificarPokemon(nombrePokemon);

	t_config* configMetadataPokemon = abrirArchivo(pathMetadataPokemon);
	char* archivoPokemonMapeado = leerArchivoPokemon(configMetadataPokemon);
	char** lineasTotales = string_split(archivoPokemonMapeado, "\n");
	cerrarArchivo(configMetadataPokemon);

	int* aImprimir = NULL;
	estructuraLocalized.cantidad_coordenadas = armarVectorCoordenadas(lineasTotales, &aImprimir);

	estructuraLocalized.nombre_pokemon = estructuraGet->nombre_pokemon;
	estructuraLocalized.coordenadas = malloc(sizeof(uint32_t) * estructuraLocalized.cantidad_coordenadas * 2);

	int j = 0;
	for(int i = 0; i < estructuraLocalized.cantidad_coordenadas; i++)
	{
		estructuraLocalized.coordenadas[i].posX = aImprimir[j];
		estructuraLocalized.coordenadas[i].posY = aImprimir[j+1];
		j+=2;
	}

	liberarArray(lineasTotales);
	free(nombrePokemon);
	free(archivoPokemonMapeado);
	free(aImprimir);
	free(pathMetadataPokemon);
	return estructuraLocalized;
}

t_caughtPokemon_msg procesarCatchPokemon(t_catchPokemon_msg* estructuraCatch)
{
	char* nombrePokemon = arreglarNombrePokemon(estructuraCatch->nombre_pokemon);

	t_caughtPokemon_msg estructuraCaught;

	if(!existePokemon(nombrePokemon))
	{
		estructuraCaught.atrapado = -1;
		return estructuraCaught;
	}

	char* pathMetadataPokemon = verificarPokemon(nombrePokemon);

	t_config* configMetadataPokemon = abrirArchivo(pathMetadataPokemon);

	char* archivoPokemonMapeado = leerArchivoPokemon(configMetadataPokemon);
	char** lineasTotales = string_split(archivoPokemonMapeado, "\n");
	char* coordenadaABuscar = string_from_format("%d-%d=", estructuraCatch->coordenadas.posX, estructuraCatch->coordenadas.posY);

	int indiceCoordenada = existeCoordenada(lineasTotales, coordenadaABuscar);
	if(indiceCoordenada < 0)
	{
		estructuraCaught.atrapado = -2;
		cerrarArchivo(configMetadataPokemon);
		return estructuraCaught;
	}

	char** coordenadaAModificar = string_split(lineasTotales[indiceCoordenada], "=");
	int cantidadPokemon = atoi(coordenadaAModificar[1]);

	if(cantidadPokemon == 1)
	{
		char* archivoNuevoPokemon = borrarLinea(indiceCoordenada, lineasTotales);
		borrarBloques(configMetadataPokemon);
		if(strlen(archivoNuevoPokemon) != 0)
			escribirArchivoPokemon(archivoNuevoPokemon, configMetadataPokemon);
		free(archivoNuevoPokemon);
	}
	else
	{
		rearmarArchivosPokemon(lineasTotales, configMetadataPokemon, indiceCoordenada, -1);
	}
	cerrarArchivo(configMetadataPokemon);


	free(nombrePokemon);
	liberarArray(coordenadaAModificar);
	free(archivoPokemonMapeado);
	liberarArray(lineasTotales);
	free(coordenadaABuscar);
	free(pathMetadataPokemon);

	estructuraCaught.atrapado = 1;

	return estructuraCaught;
}





