#include <stdlib.h>
#include <unistd.h>

#include "readLine.h"

/**
 * Lê uma linha até ao tamanho máximo maxBytes
 */
ssize_t readLine(int fd, char* buffer, long maxBytes) {

	long bytesRead = 0;
	ssize_t i;
	char c;

	do {
		i = read(fd, &c, 1);

		if (i > 0) {
			buffer[bytesRead] = c;
			bytesRead++;
		} else if (i < 0) {
			buffer[bytesRead] = '\0';
			return i;
		} else {
			return i;
		}

	} while ( i > 0 && bytesRead < maxBytes && c != '\n');

	buffer[bytesRead] = '\0';

	// Devolver o numero de bytes lidos
	return bytesRead;
}
