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

	return -1;
}

long getElementValue(char* string, long n, char* buffer, long bufferSize) {

	long i = getIndexOfElement(string, n);

	if (i < 0) {
		return 0; // Retornar 0 significa que houve algum problema
	}

	long counter = 0;

	while (string[i] != '\n' && string[i] != '\0' && string[i] != ':' && counter < bufferSize) {

		buffer[counter] = string[i];

		i++;
		counter++;
	}

	return counter;
}
