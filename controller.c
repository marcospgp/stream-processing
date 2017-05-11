#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#include "includes/globals.h"
#include "includes/readLine.h"

/* int open(const char *path, int oflag [, mode]);
ssize_t read(int fildes, void *buf, size_t nbyte);
ssize_t write(int fildes, const void *buf, size_t nbyte);
int close(int fildes); */

int createNamedPipe(int pipeId) {

	int fd;

	// Converter pipeId para string
	char pipeIdStr[256];
	sprintf(pipeIdStr, "%d", pipeId);

	// Criar a named pipe
	int result = mkfifo(pipeIdStr, 0666);

	if (result != 0) {
		fprintf(stderr, "(controller) Error creating named pipe (mkfifo() returned %d)\n", result);
		exit(EXIT_FAILURE);
	}

	fd = open(myfifo, O_WRONLY);
	write(fd, "Hi", sizeof("Hi"));
	close(fd);

	/* remove the FIFO */
	unlink(myfifo);

	return 0;

}

int closeNamedPipe(int pipeId) {

}

int main(int argc, char **argv) {

	/* Comandos que o controlador pode receber (através do ficheiro de configuração ou do stdin):
	 * node <id> <cmd> <args...>
	 *     define um nó na rede de processamento
	 * connect <id> <ids...>
	 *     permite definir ligações entre nós
	 * disconnect <id1> <id2>
	 *     permite desfazer ligações entre nós
	 * inject <id> <cmd> <args...>
	 *     permite injetar na entrada do nó id a saída produzida pelo
	 *     comando cmd executado com a lista de argumentos args
	 */

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


}
