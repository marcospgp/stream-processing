#include <string.h>

#include "globals.h"

long getIndexOfElement(char* string, long n) {

	if (n < 1) {
		return -1;
	}

	int counter = 0; // Conta o número de dois pontos (:) encontrados (a cada momento estamos no elemento counter + 1)
	char c;

	long i;
	for (i = 0; i < strlen(string); i++) {

		c = string[i];

		if ( (counter + 1) == n ) {

			return i;

		} else if (c == ':') {

			counter++;
		}
	}
}
