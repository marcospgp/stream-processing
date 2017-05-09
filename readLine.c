#include <stdlib.h>
#include <unistd.h>

#include "readLine.h"

/**
 * Lê uma linha até ao tamanho máximo maxBytes
 */
ssize_t readln (int fd, char* buffer, size_t maxBytes) {

	long bytesread = 0;
	ssize_t i;
	char c;

	do {
		i = read(fd, &c, 1);

		if (i > 0) {
			buffer[bytesread] = c;
			bytesread++;
		} else if (i < 0) {
			buffer[bytesRead] = '\0';
			return i;
		} else {
			return i;
		}

	} while ( i > 0 && bytesread < maxBytes && c != '\n');

	buffer[bytesread] = '\0';

	// Devolver o numero de bytes lidos
	return bytesread;
}
