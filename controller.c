#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "includes/globals.h"
#include "includes/readLine.h"

/* int open(const char *path, int oflag [, mode]);
ssize_t read(int fildes, void *buf, size_t nbyte);
ssize_t write(int fildes, const void *buf, size_t nbyte);
int close(int fildes); */

static int nodes[MAX_NODES];                  // nodes[nodeId] = processID
static int connections[MAX_NODES][MAX_NODES]; // connections[from][to] = processID

static void closeNamedPipePair(int pipeId) {

	char writePipe[256];
	char readPipe[256];

	getWritePipeStr(pipeId, writePipe);
	getReadPipeStr(pipeId, readPipe);

	if (access(writePipe, F_OK) == -1 || access(readPipe, F_OK) == -1) {
		return;
	}

	unlink(writePipe);
	unlink(readPipe);

	return;
}

static int createNamedPipePair(int pipeId) {

	char writePipe[256];
	char readPipe[256];

	getWritePipeStr(pipeId, writePipe);
	getReadPipeStr(pipeId, readPipe);

	// Verificar se a named pipe já existe
	if (access(writePipe, F_OK) == 0 || access(readPipe, F_OK) == 0) {
		fprintf(stderr, "Creating a FIFO pair that already exists, going to delete first\n");

		closeNamedPipePair(pipeId);
	}

	// Criar a named pipe

	printf("Creating pipes %s and %s\n", writePipe, readPipe);

	int resultW = mkfifo(writePipe, 0666);
	int resultR = mkfifo(readPipe, 0666);

	if (resultW != 0 || resultR != 0) {
		fprintf(stderr, "Error creating named pipe for node %d (mkfifo() returned %d (write) %d (read)) Error message: %s\n", pipeId, resultW, resultR, strerror(errno));
		return -1;
	}

	return 0;
}

static int createNode(int nodeId, char* cmd, char** args) {

	printf("Creating node %d with cmd %s and args ", nodeId, cmd);

	int i;
	for (i = 0; args[i] != NULL; i++) {
		printf("%s ", args[i]);
	}

	printf("\n");

	// Criar um filho para correr o nó

	pid_t pid = fork();

	if (pid != 0) {

		nodes[nodeId] = pid;

	} else {

		// Criar named pipe para este nó
		if (createNamedPipePair(nodeId) != 0) {
			fprintf(stderr, "Failed to create named pipe\n");
			exit(EXIT_FAILURE);
		}

		// Redirecionar stderr
		int fd = open("log.txt", O_WRONLY|O_APPEND|O_CREAT, 0666);
		dup2(fd, 2);

		// Converter pipeId para string
		char writePipe[256];
		char readPipe[256];

		getWritePipeStr(nodeId, writePipe);
		getReadPipeStr(nodeId, readPipe);

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

		char* args2[sizeof(args) + sizeof(char*)];

		args2[0] = cmd;

		int k;
		for (k = 0; args[k] != NULL; k++) {
			args2[k + 1] = args[k];
		}

		execv(cmd, args2);

		fprintf(stderr, "(controller) Node creation failed (execv(%s, [...]))\n", cmd);

		close(resW);
		close(resR);

		closeNamedPipePair(nodeId);

		// Se o execv falhou, terminar com código 127
		exit(127);
	}

	return 0;
}

static int createConnection(int id, char** args) {

	// Ir buscar a pipe onde o nó escreve
	char fifoFrom[256];
	getWritePipeStr(id, fifoFrom);

	// Ir buscar as pipes onde os outros nós vão ler

	int j;
	for (j = 0; args[j] != NULL; j++) {

		int nodeToId = (int) strtol(args[j], (char**) NULL, 10);

		printf("Connecting node %d to %d\n", id, nodeToId);

		char fifoTo[256];
		getReadPipeStr(nodeToId, fifoTo);

		int pid = fork();

		if (pid != 0) {

			connections[id][nodeToId] = pid;

		} else {

			// Redirecionar stdout e stderr
			int fd = open("log.txt", O_WRONLY|O_APPEND|O_CREAT, 0666);
			dup2(fd, 1);
			dup2(fd, 2);

			if (WINDOWS_MODE) {

				char* argsToPass[] = {"connect.exe", fifoFrom, fifoTo};

				execv("connect.exe", argsToPass);

				fprintf(stderr, "(controller) Connection creation failed (execv(%s, [%s, %s])) Error message: %s\n", "connect.exe", fifoFrom, fifoTo, strerror(errno));

			} else {

				char* argsToPass[] = {"connect", fifoFrom, fifoTo};

				execv("connect", argsToPass);

				fprintf(stderr, "(controller) Connection creation failed (execv(%s, [%s, %s])) Error message: %s\n", "connect", fifoFrom, fifoTo, strerror(errno));
			}

			// Se o execv falhou, terminar com código 127
			exit(127);
		}
	}

	return 0;
}

static int removeConnection(int from, int to) {

	if (connections[from][to] != 0) {

		kill(connections[from][to], SIGKILL);
	}

	return 0;
}

static int removeAllConnections() {

	int i, j;
	for (i = 0; i < MAX_NODES; i++) {

		for (j = 0; j < MAX_NODES; j++) {

			if (connections[i][j] != 0) {

				kill(connections[i][j], SIGKILL);
			}
		}
	}

	return 0;
}

static int removeNode(int id) {

	if (nodes[id] != 0) {
		kill(nodes[id], SIGKILL);
	}

	closeNamedPipePair(id);

	return 0;
}

static int removeAllNodes() {

	int i;
	for (i = 0; i < MAX_NODES; i++) {

		if (nodes[i] != 0) {

			printf("Removing node %d with pid %d\n", i, nodes[i]);
			kill(nodes[i], SIGKILL);
		}

		closeNamedPipePair(i);
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

	// Inicializar arrays a -1
	memset(connections, 0, sizeof(connections[0][0]) * MAX_NODES * MAX_NODES);
	memset(nodes, 0, sizeof(nodes));

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

					createConnection(id, args);

				} else if (strcmp(cmd, "disconnect") == 0) {

					int to = (int) strtol(args[0], (char**) NULL, 10);

					removeConnection(id, to);

				} else if (strcmp(cmd, "inject") == 0) {

					// TODO - inject

				} else {

					fprintf(stderr, "Skipping unknown command in configuration file (%s)\n", cmd);
				}
			}

			close(fd);

			if (i < 0) {

				fprintf(stderr, "Error reading configuration line: %s (readLine() returned %d)\n", strerror(errno), i);
				return EXIT_FAILURE;

			}
		}
	}

	char buffer2[PIPE_BUF];
	int j;
	while ((j = readLine(0, buffer2, (long) PIPE_BUF)) > 0) {

		if (strcmp(buffer2, "stop\n") == 0) {

			printf("Removing all connections and nodes...\n");
			removeAllConnections();
			removeAllNodes();
		}
	}

	if (j < 0) {
		fprintf(stderr, "Error reading user input: %s (readLine returned %d)\n", strerror(errno), j);
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}
