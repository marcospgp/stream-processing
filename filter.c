#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "globals.h"
#include "readLine.h"

int main (int argc, char** argv) {

	// Chamada com o formato "filter colunaA operador colunaB", exemplo: "filter 2 > 4"
	// Operadores possíveis: =, >=, <=, >, <, !=

	char* end;

	long columnA = strtol(argv[1], &end, 10);
	char operator = argv[2];
	long columnB = strtol(argv[3], &end, 10);

	// strtol error handling (strtol retorna 0 quando há um erro)
	if ( (columnA == 0 || columnB == 0) && errno != 0 ) {

		fprintf(stderr, "(filter) Error reading arguments: %s\n", strerror(errno));
		return EXIT_FAILURE;

	} else if (columnA < 1 || columnB < 1) {

		fprintf(stderr, "(filter) Error reading arguments (column value < 1): %ld %ld\n", columnA, columnB);
		return EXIT_FAILURE;
	}

	// Ler linhas do stdin

	char* buffer[PIPE_BUF];
	char* columnAValue = (char*) malloc(PIPE_BUF);
	char* columnBValue = (char*) malloc(PIPE_BUF);
	long columnAValueLong;
	long columnBValueLong;
	char* end2;

	int i;

	while ( (i = readLine(0, buffer, PIPE_BUF)) > 0) {

		// Obter valores das colunas
		columnAValue = getElementValue(buffer, columnA, columnAValue, PIPE_BUF);
		columnBValue = getElementValue(buffer, columnB, columnBValue, PIPE_BUF);

		if (columnAValue < 1 || columnBValue < 1) {

			fprintf(stderr, "(filter) Error obtaining column values (getElementValue returned 0)");
			return EXIT_FAILURE;
		}

		columnAValueLong = strtol(columnAValue, &end2, 10);
		columnBValueLong = strtol(columnBValue, &end2, 10);

		if ( (columnAValueLong == 0 || columnBValueLong) && errno != 0 ) {

			fprintf(stderr, "(filter) Error converting column values to long (strtol returned 0 and set errno)");
			return EXIT_FAILURE;
		}

		// Determinar operador a usar

	}

	if (i < 0) {

		fprintf(stderr, "(filter) Error reading input: %s (read() returned %d)\n", strerror(errno), i);
		return EXIT_FAILURE;

	} else {

		return EXIT_SUCCESS;
	}
}
