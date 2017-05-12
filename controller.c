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

	char* writePipe = getWritePipeStr(pipeId);
	char* readPipe = getReadPipeStr(pipeId);

	// Verificar se a named pipe já existe
	if (access(writePipe, F_OK) == 0 || access(readPipe, F_OK) == 0) {
		fprintf(stderr, "(controller) Trying to create a FIFO pair that already exists\n");
	}

	// Criar a named pipe

	printf("(controller) Trying to create pipes %s and %s\n", writePipe, readPipe);

	int resultW = mkfifo(writePipe, 0777);
	int resultR = mkfifo(readPipe, 0777);

	if (resultW != 0 || resultR != 0) {
		fprintf(stderr, "(controller) Error creating named pipe (mkfifo() returned %d (write) %d (read))\n", resultW, resultR);
		return 0;
	}

	return 1;
}

static void closeNamedPipe(int pipeId) {

	char* writePipe = getWritePipeStr(pipeId);
	char* readPipe = getReadPipeStr(pipeId);

	if (access(writePipe, F_OK) == -1 || access(readPipe, F_OK) == -1) {
		return;
	}

	unlink(writePipe);
	unlink(readPipe);

	return;
}

static int createNode(int nodeId, char* cmd, char** args) {

	// Criar um filho para correr o nó

	pid_t pid = fork();

	if (pid == 0) {

		// Criar named pipe para este nó
		if (!createNamedPipe(nodeId)) {
			fprintf(stderr, "(controller) Failed to create named pipe\n");
			return EXIT_FAILURE;
		}

		// Converter pipeId para string
		char* writePipe = getWritePipeStr(nodeId);
		char* readPipe = getReadPipeStr(nodeId);

		// Trocar stdout e stdin deste processo por named pipes

		int resW = open(writePipe, O_WRONLY);
		int resR = open(readPipe, O_RDONLY);

		dup2(resR, 0);
		dup2(resW, 1);

		// Adicionar sufixo .exe quando estivermos a trabalhar em windows
		if (WINDOWS_MODE && (
			strcmp(cmd, "filter") == 0 ||
			strcmp(cmd, "const")  == 0 ||
			strcmp(cmd, "window") == 0 ||
			strcmp(cmd, "spawn")  == 0
		)) {
			cmd = strcat(cmd, ".exe");
		}

		execv(cmd, args);

		fprintf(stderr, "(controller) Node creation failed (execv(%s, [...]))\n", cmd);

		close(resW);
		close(resR);

		closeNamedPipe(nodeId);

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

				printf("Reading configuration line: %s", buffer);

				char** tokens = split(buffer, ' ');

				// Get cmd
				char* cmd = tokens[0];

				// Get id
				int id = (int) strtol(tokens[1], (char**) NULL, 10);

				// Get argumentos restantes
				char** args = &tokens[2];

				// Switch cmd

				if (strcmp(cmd, "node") == 0) {

					createNode(id, tokens[2], &tokens[3]);

				} else if (strcmp(cmd, "connect") == 0) {

					// Ir buscar a pipe onde o nó escreve
					char* fifoFrom = getWritePipeStr(id);

					// Ir buscar as pipes onde os outros nós vão ler

					int j = 0;
					while (args[j] != NULL) {

						int nodeId = (int) strtol(args[j], (char**) NULL, 10);

						char* fifoTo = getReadPipeStr(nodeId);

						int pid = fork();

						if (pid == 0) {

							char* argsToPass[] = {fifoFrom, fifoTo};

							if (WINDOWS_MODE) {

								execv("connect.exe", argsToPass);

								fprintf(stderr, "(controller) Connection creation failed (execv(%s, [%s, %s]))\n", "connect.exe", fifoFrom, fifoTo);

							} else {

								execv("connect", argsToPass);

								fprintf(stderr, "(controller) Connection creation failed (execv(%s, [%s, %s]))\n", "connect", fifoFrom, fifoTo);
							}

							// Se o execv falhou, terminar com código 127
							exit(127);
						}
					}

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
