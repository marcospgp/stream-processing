#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "includes/globals.h"
#include "includes/readLine.h"

int main(int argc, char** argv) {


	// Chamada com o formato "window colunaA operador colunaB", exemplo: "window 4 avg 2"
	// Operadores possíveis: avg, max, min, sum 

	if (argc < 4) {
		return EXIT_FAILURE;
	}

	char* end;

	long columnA = strtol(argv[1], &end, 10);
	char* operator = argv[2];
	long columnB = strtol(argv[3], &end, 10);

	// Cria um array com o número de posições dado pelo 2º argumento da funcao
	long array[columnB];

	// strtol error handling (strtol retorna 0 quando há um erro)
	if ( (columnA == 0 || columnB == 0) && errno != 0 ) {

		fprintf(stderr, "(filter) Error reading arguments: %s\n", strerror(errno));
		return EXIT_FAILURE;

	} else if (columnA < 1 || columnB < 1) {

		fprintf(stderr, "(filter) Error reading arguments (column value < 1): %ld %ld\n", columnA, columnB);
		return EXIT_FAILURE;
	}

	// Ler linhas do stdin

	char buffer[PIPE_BUF];
	char* columnAValue = (char*) malloc(PIPE_BUF);

	ssize_t i;
	int j = 0;
	long maxsize = columnB;
	char *output = malloc(PIPE_BUF);
	int flag = 1;
	long firstvalue;

	while ( (i = readLine(0, buffer, (long) PIPE_BUF)) > 0) {

		// Obter valores das colunas
		if (
			getElementValue(buffer, columnA, columnAValue, (long) PIPE_BUF) < 1
		) {
			fprintf(stderr, "(filter) Error obtaining column values (getElementValue returned 0)");
			return EXIT_FAILURE;
		}

		long columnAValueLong;
		char* end2;

		columnAValueLong = strtol(columnAValue, &end2, 10);

		if ( (columnAValueLong == 0) && errno != 0 ) {

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

				for(int i = 0; array[i]!='\0'; i++) {
					sum += array[i];
				}
					avg = sum / maxsize;
				sprintf(output,"%lu", avg);

			} else if (strcmp(operator, "max") == 0) {

				for(int i = 0; array[i] != '\0'; i++) {
					if (array[i] > larger) larger = array[i];
				}
				sprintf(output,"%lu", larger);

			} else if (strcmp(operator, "min") == 0) {

				for(int i = 0; array[i] != '\0'; i++) {
					if (array[i] < smaller) smaller = array[i];
				}
				sprintf(output,"%lu", smaller);

			} else if (strcmp(operator, "sum") == 0) {

				for(int i = 0; array[i]!='\0'; i++){
					sum += array[i];
				}
				sprintf(output,"%lu", sum);
			}
		} else {
			if (flag == 1) {
				firstvalue = columnAValueLong;
				sprintf(output,"%lu", firstuse);
				flag = 2;
			} 
			else {
				sprintf(output,"%lu", firstvalue);
				flag = 0;
			}

		}
		

		if (j == maxsize) {
			array[0] = columnAValueLong;
			j = 0;
			j++;
		}
		else {
			array[j] = columnAValueLong;
			j++;
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