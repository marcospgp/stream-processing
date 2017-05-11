#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>

#include "globals.h"

// String to long
// Retorna 1 se a conversão foi bem sucedida, caso contrário retorna 0
int parseLong(const char* str, long* val) {

    char *temp;
    int rc = 1;
    errno = 0;
    *val = strtol(str, &temp, 0);

    if (temp == str || *temp != '\0' ||
        ((*val == LONG_MIN || *val == LONG_MAX) && errno == ERANGE))
        rc = 0;

    return rc;
}

// Duplica uma string
char* duplicateString(char* org) {

    int org_size;
    static char *dup;
    char *dup_offset;

    /* Allocate memory for duplicate */

    org_size = strlen(org);

    dup = (char*) malloc(sizeof(char)*org_size+1);

    if( dup == NULL) {
        return( (char*) NULL);
    }

    /* Copy string */

    dup_offset = dup;

    while(*org) {
        *dup_offset = *org;
        dup_offset++;
        org++;
    }

    *dup_offset = '\0';

    return(dup);
}

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

	buffer[counter] = '\0';

	return counter;
}

// Array dinâmico

void initArray(Array *a) {

	size_t initialSize = INITIAL_ARRAY_SIZE;

	a->array = (int *)malloc(initialSize * sizeof(int));
	a->used = 0;
	a->size = initialSize;
}

void insertArray(Array *a, int element) {
	// a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
	// Therefore a->used can go up to a->size
	if (a->used == a->size) {
		a->size *= 2;
		a->array = (int *)realloc(a->array, a->size * sizeof(int));
	}
	a->array[a->used++] = element;
}

void freeArray(Array *a) {
	free(a->array);
	a->array = NULL;
	a->used = a->size = 0;
}

// Divide uma string em várias com base num delimitador. O array resultante termina em NULL
char** split(char* a_str, const char a_delim) {

	char** result    = 0;
	size_t count     = 0;
	char* tmp        = a_str;
	char* last_comma = 0;
	char delim[2];
	delim[0] = a_delim;
	delim[1] = 0;

	/* Count how many elements will be extracted. */
	while (*tmp) {

		if (a_delim == *tmp) {

			count++;
			last_comma = tmp;
		}

		tmp++;
	}

	/* Add space for trailing token. */
	count += last_comma < (a_str + strlen(a_str) - 1);

	/* Add space for terminating null string so caller
	   knows where the list of returned strings ends. */
	count++;

	result = malloc(sizeof(char*) * count);

	if (result) {

		size_t idx  = 0;
		char* token = strtok(a_str, delim);

		while (token) {

			assert(idx < count);
			*(result + idx++) = duplicateString(token);
			token = strtok(0, delim);
		}

		assert(idx == count - 1);
		*(result + idx) = 0;
	}

	return result;
}
