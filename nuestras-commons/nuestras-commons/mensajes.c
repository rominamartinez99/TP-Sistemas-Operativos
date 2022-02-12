#include "mensajes.h"

char* op_code_a_string(op_code code)
{
	switch(code) {
		case 1: return "NEW POKEMON";
		case 2: return "APPEARED POKEMON";
		case 3: return "CATCH POKEMON";
		case 4: return "CAUGHT POKEMON";
		case 5: return "GET POKEMON";
		case 6: return "LOCALIZED POKEMON";
		default: return "";
	}
}
