#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "includes/globals.h"
#include "includes/readLine.h"

int main(int argc, char** argv) {

	if (argc < 2) {
		return EXIT_FAILURE;
	}

	long input[128];
	char* end;
	long read;
	int* positions = malloc(sizeof(int)*1);
	int counter = 0;

	for (int i = 1, j = 0; i < argc; i++, j++) {
	 	if (argv[i][0] == '$') {
	 		read = strtol(&(argv[i][1]), &end, 10);
	 		if (read == 0 && errno == 0) {
				fprintf(stderr, "Error reading arguments: %s\n", strerror(errno));
				j--;
			}
			else {
	 			input[j] = read;
	 			counter++;
	 			*(positions+j) = i;
	 			realloc(positions,sizeof(int)*1);
	 		}
	 	}
	 }

	// Ler linhas do stdin

	char buffer[PIPE_BUF];
	char* columnAValue = malloc(PIPE_BUF);

	ssize_t i;
	while ( (i = readLine(0, buffer, (long) PIPE_BUF)) > 0) {

		// Obter valores das colunas
		for(int d = 0; d < counter; d++) {
			if (getElementValue(buffer, input[d], &columnAValue[d], (long) PIPE_BUF) < 1) {
				fprintf(stderr, "(filter) Error obtaining column values (getElementValue returned 0)");
				return EXIT_FAILURE;
			}
			else {
				strcpy(argv[*(positions+d)], &columnAValue[d]);
			}
		}

		char *output = malloc(PIPE_BUF);

		int exitStatus;
		int result;
		pid_t childId = fork();

		if (childId == 0) {
			execvp(argv[1], &argv[1]);
		}

		while (wait(&exitStatus) > 0) {
			result = WEXITSTATUS(exitStatus);
		}

		sprintf(output, "%d", result);

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