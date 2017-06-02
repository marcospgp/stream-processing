#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include <ctype.h>


#include "includes/globals.h"
#include "includes/readLine.h"

int main(int argc, char** argv) {

	// Chamada com o formato "window coluna operador linhas", exemplo: "window 4 avg 2"
	// Operadores possíveis: avg, max, min, sum

	if (argc < 4) {
		return EXIT_FAILURE;
	}

	char* end;

	long column = strtol(argv[1], &end, 10);
	char* operator = argv[2];
	long lines = strtol(argv[3], &end, 10);

	// Cria um array com o número de posições dado pelo 2º argumento da funcao
	long array[lines];

	// Poe o array a zeros
	memset(array,0,sizeof(array));

	// strtol error handling (strtol retorna 0 quando há um erro)
	if ( (column == 0 || lines == 0) && errno != 0 ) {

		fprintf(stderr, "(filter) Error reading arguments: %s\n", strerror(errno));
		return EXIT_FAILURE;

	} else if (column < 1 || lines < 1) {

		fprintf(stderr, "(filter) Error reading arguments (column value < 1): %ld %ld\n", column, lines);
		return EXIT_FAILURE;
	}

	// Ler linhas do stdin

	char buffer[PIPE_BUF];
	char* columnValue = (char*) malloc(PIPE_BUF);

	ssize_t i;
	int j = 0;
	long maxsize = lines;
	char *output = malloc(PIPE_BUF);
	int flag = 1;
	long firstvalue = 0;
	int counter = 0;

	while ( (i = readLine(0, buffer, (long) PIPE_BUF)) > 0) {

		// Obter valores das colunas
		if (
			getElementValue(buffer, column, columnValue, (long) PIPE_BUF) < 1
		) {
			fprintf(stderr, "(filter) Error obtaining column values (getElementValue returned 0)");
			return EXIT_FAILURE;
		}

		long columnValueLong;
		char* end2;

		columnValueLong = strtol(columnValue, &end2, 10);

		if ( (columnValueLong == 0) && errno != 0 ) {

			fprintf(stderr, "(filter) Error converting column values to long (strtol returned 0 and set errno)");
			return EXIT_FAILURE;
		}

		// Determinar operador a usar
		long sum = 0;
		long avg = 0;
		long larger = 0;
		long smaller = +9223372036854775807;
		long firstuse = 0;

		if (flag == 0) {
			if (strcmp(operator, "avg") == 0) {

				for(int i = 0; i < maxsize; i++) {
					sum += array[i];
				}
					avg = sum / maxsize;
				sprintf(output,"%lu", avg);

			} else if (strcmp(operator, "max") == 0) {

				for(int i = 0; i < maxsize; i++) {
					if (array[i] > larger) larger = array[i];
				}
				sprintf(output,"%lu", larger);

			} else if (strcmp(operator, "min") == 0) {

				for(int i = 0; i < counter; i++) {
					if (array[i] < smaller) smaller = array[i];
				}
				sprintf(output,"%lu", smaller);

			} else if (strcmp(operator, "sum") == 0) {

				for(int i = 0; i < maxsize; i++){
					sum += array[i];
				}
				sprintf(output,"%lu", sum);
			}
		} else {
			if (flag == 1) {
				firstvalue = columnValueLong;
				sprintf(output,"%lu", firstuse);
				flag = 2;
			}
			else {
				sprintf(output,"%lu", firstvalue);
				flag = 0;
			}
		}

		if (j == maxsize) {
			array[0] = columnValueLong;
			j = 0;
			j++;
		}
		else {
			array[j] = columnValueLong;
			j++;
			if (counter != 4) counter++;
		}


		buffer[i - 1] = ':';
		buffer[i] = '\0';

		strcat(buffer, output);

		ssize_t newLength = i + strlen(output);

		buffer[newLength] = '\n';
		buffer[newLength + 1] = '\0';

		newLength++;

		write(1, buffer, newLength);
	}

	if (i < 0) {

		fprintf(stderr, "(filter) Error reading input: %s (read() returned %ld)\n", strerror(errno), i);
		return EXIT_FAILURE;

	} else {

		return EXIT_SUCCESS;
	}
}