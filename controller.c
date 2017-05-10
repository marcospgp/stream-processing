#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "includes/globals.h"
#include "includes/readLine.h"

/* int open(const char *path, int oflag [, mode]);
ssize_t read(int fildes, void *buf, size_t nbyte);
ssize_t write(int fildes, const void *buf, size_t nbyte);
int close(int fildes); */

int main (int argc, char **argv) {

	// O controlador pode receber um argumento que é o caminho de um ficheiro de configuração
	if (argc > 1) {

		int cfgd = fopen(argv[1], "r");

		if (cfgd) {

			char buffer[PIPE_BUF];

		    while ((i = readLine(cfgd, buffer, (long) PIPE_BUF)) > 0) {

		    	// TODO - execute config lines
		    	printf("Reading configuration line\n\n%s\n\n", buffer);
		    }

			fclose(file);

		} else {
			fprintf(stderr, "(controller) Error opening configuration file\n");
			return EXIT_FAILURE;
		}
	}

	int fd;
	char* myfifo = "/tmp/myfifo";

	/* create the FIFO (named pipe) */
	mkfifo(myfifo, 0666);

	/* write "Hi" to the FIFO */
	fd = open(myfifo, O_WRONLY);
	write(fd, "Hi", sizeof("Hi"));
	close(fd);

	/* remove the FIFO */
	unlink(myfifo);

	return 0;
}

