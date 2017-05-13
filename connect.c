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

	if (argc < 3) {
		fprintf(stderr, "(connect) Tried to create a connection but didn't provide enough arguments\n");
		return EXIT_FAILURE;
	}

	char* pipeFrom = argv[1];
	char* pipeTo = argv[2];

	// Ler linhas do stdin

	char buffer[PIPE_BUF];
	ssize_t i;

	int from = open(pipeFrom, O_RDONLY);
	int to = open(pipeTo, O_WRONLY);

	if (from < 0 || to < 0) {

		fprintf(stderr, "(connect) Error opening pipes (open(from) returned %d and open(to) returned %d)\n", from, to);
		return EXIT_FAILURE;
	}

	printf("(connect) Connection from pipe %s to %s established.\n", argv[1], argv[2]);

	while ( (i = readLine(from, buffer, (long) PIPE_BUF)) > 0) {

		write(to, buffer, strlen(buffer));
	}

	if (i < 0) {

		fprintf(stderr, "(connect) Error reading input: %s (read() returned %ld)\n", strerror(errno), i);
		return EXIT_FAILURE;

	} else {

		return EXIT_SUCCESS;
	}
}
