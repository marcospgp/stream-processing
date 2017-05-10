#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "includes/globals.h"
#include "includes/readLine.h"

/* int open(const char *path, int oflag [, mode]);
ssize_t read(int fildes, void *buf, size_t nbyte);
ssize_t write(int fildes, const void *buf, size_t nbyte);
int close(int fildes); */


// argv[1] eh o 10
// minibuf e a:3:x:4

int main (int argc, char **argv) {

	if (argc < 2) {
		return EXIT_FAILURE;
	}

	char *input = argv[1];
	char *minibuf = malloc(PIPE_BUF);

	ssize_t bufferSize = PIPE_BUF - 1 - strlen(input);
	ssize_t i;

	while ( (i = readLine(0, minibuf, bufferSize)) > 0) {

		minibuf[i - 1] = ':';
		minibuf[i] = '\0';

		strcat(minibuf, input);

		ssize_t newLength = i + strlen(input);

		minibuf[newLength] = '\n';
		minibuf[newLength + 1] = '\0';

		newLength++;

		write(1, minibuf, newLength);
	}

	if (i < 0) {

		fprintf(stderr, "(const) Error reading input: %s (read() returned %ld)\n", strerror(errno), i);
		return EXIT_FAILURE;

	} else {

		return EXIT_SUCCESS;
	}
}
