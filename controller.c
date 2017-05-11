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

/* int open(const char *path, int oflag [, mode]);
ssize_t read(int fildes, void *buf, size_t nbyte);
ssize_t write(int fildes, const void *buf, size_t nbyte);
int close(int fildes); */

static int createNamedPipe(int pipeId) {

	// Converter pipeId para string
	char pipeIdStr[256];
	sprintf(pipeIdStr, "%d", pipeId);

	// Criar a named pipe
	int result = mkfifo(pipeIdStr, 0666);

	if (result != 0) {
		fprintf(stderr, "(controller) Error creating named pipe (mkfifo() returned %d)\n", result);
		exit(EXIT_FAILURE);
	}

	int fd = open(pipeIdStr, O_WRONLY);
	write(fd, "Hi", sizeof("Hi"));
	close(fd);

	return 0;
}

static int closeNamedPipe(int pipeId) {

	// Converter pipeId para string
	char pipeIdStr[256];
	sprintf(pipeIdStr, "%d", pipeId);

	unlink(pipeIdStr);

	return 0;
}

static int createNode(int nodeId, char* cmd, char** args) {

	// Criar um filho para correr o nó

	pid_t pid = fork();

	if (pid == 0) {

		// Criar named pipe para este nó
		//createNamedPipe(nodeId, )

		execv(cmd, args);

		// Se o execv falhou, terminar com código 127
		exit(127);
	}

	return 0;
}

int main(int argc, char** argv) {

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

		int fd = open(argv[1], O_RDONLY);

		if (fd < 0) {

			printf("Could not find configuration file. Skipping (open() returned %d)\n", fd);

		} else {

			char buffer[PIPE_BUF];

			int i;
			while ((i = readLine(fd, buffer, (long) PIPE_BUF)) > 0) {

				printf("Reading configuration line\n\n%s\n\n", buffer);

				char** tokens = split(buffer, ' ');

				// Get cmd
				char* cmd = tokens[0];

				// Get id
				int id = (int) strtol(tokens[1], (char**) NULL, 10);

				// Get argumentos restantes
				char** args = &tokens[2];

				printf("cmd: %s\n", cmd);

				printf("id: %d\n", id);

				int j;
				for (j = 0; *(args + j); j++) {
					printf("arg: %s\n", args[j]);
				}

				return 0;

				// Switch cmd

				if (strcmp(cmd, "node") == 0) {

					createNode(id, cmd, args);

				} else if (strcmp(cmd, "connect") == 0) {

					// TODO - connect

				} else if (strcmp(cmd, "disconnect") == 0) {

					// TODO - disconnect

				} else if (strcmp(cmd, "inject") == 0) {

					// TODO - inject

				} else {

					fprintf(stderr, "(controller) Skipping unknown command in configuration file (%s)\n", cmd);
				}
			}

			close(fd);

			if (i < 0) {

				fprintf(stderr, "(controller) Error reading configuration line: %s (readLine() returned %d)\n", strerror(errno), i);
				return EXIT_FAILURE;
			}

		}
	}
}
