#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include "includes/globals.h"
#include "includes/readLine.h"

// Faz uma ligação entre duas named pipes
int main(int argc, char** argv) {

	// Se argc < 2, este connect só tem de mandar o que lê para o stdout
	// (provavelmente é uma ponte entre um nó e um inject, e o childCreator
	// já trocou o seu stdout por uma named pipe)

	// Obter as input pipes dos nodes que vão ouvir

	char* toPipes[argc - 1];
	int toPipesFD[argc - 1];
	int numListeners = 0;

	if (argc > 1) {


		int k;
		for (k = 0; toPipes[k] != NULL; k++) {

			int nodeToId = (int) strtol(argv[k], (char**) NULL, 10);

			char pipeTo[256];
			getReadPipeStr(nodeToId, pipeTo);

			toPipes[k] = pipeTo;

			int to = open(pipeTo, O_WRONLY);

			if (to < 0) {
				fprintf(stderr, "(connect) Error opening listener pipe %s (open returned %d)\n", pipeTo, to);
				return EXIT_FAILURE;
			}

			toPipesFD[k] = to;
		}

		// Compiler says this variable is not used, but it is -_-
		// (inside the condition on a for loop below)
		numListeners = k;
	}

	// Ler linhas do stdin

	char readBuffer[PIPE_BUF];
	ssize_t i;

	printf("(connect) Connection from pipe %s to listeners established.\n", argv[1]);

	while ( (i = readLine(0, readBuffer, (long) PIPE_BUF)) > 0 ) {

		// Escrever input para todos os ouvintes
		if (argc > 1) {

			int l;
			for (l = 0; l < numListeners; l++) {
				write(toPipesFD[l], readBuffer, strlen(readBuffer));
			}

		} else {
			write(1, readBuffer, strlen(readBuffer));
		}
	}

	if (i < 0) {

		fprintf(stderr, "(connect) Error reading input: %s (read() returned %ld)\n", strerror(errno), i);
		return EXIT_FAILURE;

	} else {

		return EXIT_SUCCESS;
	}
}
