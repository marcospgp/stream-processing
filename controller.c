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

static int nodes[MAX_NODES]; // nodes[nodeId] = processID (0 if it doesn't exist)

// Connection proceses - each node has one that copies its output as many times as needed
// and sends it to all the listening nodes
static int connections[MAX_NODES]; // connections[from] = processID (0 if it doesn't exist)

// Inject connection processes - each inject process has one that sends its output to one listening node
static int injectConnections[MAX_INJECTS]; // injectConnections[injId] = processID (0 if it doesn't exist)

// We need the injectPipes array to keep track of the available ids for injects
static int injectPipes[MAX_INJECTS]; // injectPipes[injectId] = 1 if pipe exists, 0 if it doesn't

// The connectionDests array keeps track of what nodes are listening to the output of each node
static int connectionDests[MAX_NODES][MAX_OUTGOING_CONNECTIONS] // connectionDests[nodeId] = listeningNodeIds[]

/**
 * Closes a pair of named pipes (FIFOs)
 */
static void closeNamedPipePair(int pipeId) {

	char writePipe[256];
	char readPipe[256];

	getWritePipeStr(pipeId, writePipe);
	getReadPipeStr(pipeId, readPipe);

	// Check that pipes exist first
	if (access(writePipe, F_OK) != -1) {
		unlink(writePipe);
	}

	if (access(readPipe, F_OK) != -1) {
		unlink(readPipe);
	}

	return;
}

/**
 * Closes a named pipe used by an inject process
 */
static void closeInjectPipe(int pipeId) {

	char pipe[256];

	getInjectPipeStr(pipeId, pipe);

	// Check that pipe exists
	if (access(pipe, F_OK) != -1) {
		unlink(pipe);
	}

	// Signal that this id is now available
	injectPipes[pipeId] = 0;

	return;
}

/**
 * Creates a pair of names pipes (FIFOs) through which
 * the input and output of a node will pass
 * (note that the output will always go to a connection process
 * so that we can send the data to multiple nodes)
 *
 * @return 0 on success, -1 if something went wrong
 */
static int createNamedPipePair(int pipeId) {

	char writePipe[256];
	char readPipe[256];

	getWritePipeStr(pipeId, writePipe);
	getReadPipeStr(pipeId, readPipe);

	// Check if named pipes already exist
	if (access(writePipe, F_OK) == 0 || access(readPipe, F_OK) == 0) {
		fprintf(stderr, "Creating a FIFO pair that already exists, going to delete first\n");

		closeNamedPipePair(pipeId);
	}

	// Create the named pipe pair

	printf("Creating pipes %s and %s\n", writePipe, readPipe);

	int resultW = mkfifo(writePipe, 0666);
	int resultR = mkfifo(readPipe, 0666);

	if (resultW != 0 || resultR != 0) {
		fprintf(stderr, "Error creating named pipe for node %d (mkfifo() returned %d (write) %d (read)) Error message: %s\n", pipeId, resultW, resultR, strerror(errno));
		return -1;
	}

	return 0;
}

/**
 * Creates a pipe through which the output of an inject will pass
 *
 * @return  A id da pipe criada ou -1 se houver um erro
 */
static int createInjectPipe() {

	// Find available pipe id
	int i, foundFree = 0;
	for (i = 0; i < MAX_INJECTS; i++) {
		if (injectPipes[i] == 0) {
			foundFree = 1;
			injectPipes[i] = 1;
			break;
		}
	}

	if (!foundFree) {
		fprintf(stderr, "No more space for creating inject pipes\n");
		return -1;
	}

	char pipe[256];
	getInjectPipeStr(i, pipe);

	// Check if named pipe already exists
	if (access(pipe, F_OK) == 0) {
		fprintf(stderr, "Creating an inject FIFO pipe that already exists, going to delete first\n");
		closeInjectPipe(i);
	}

	// Create the named pipe

	printf("Creating inject pipe %s\n", pipe);

	int result = mkfifo(pipe, 0666);

	if (result != 0) {
		fprintf(stderr, "Error creating inject FIFO pipe (mkfifo() returned %d) Error message: %s\n", result, strerror(errno));
		return -1;
	}

	return i;
}

/**
 * Creates a connection process for a node.
 * The output of the node is copied as many times as needed and sent to the
 * stdins of all the listening nodes.
 *
 * @param id The id of the node which will send its output through this process
 * @param listeners A string array containing the id's of the listening nodes
 */
static int createConnection(int id, char** listeners) {

	// Check that sending node exists
	if (!nodes[id]) {
		fprintf(stderr, "(controller) Connection creation failed - writing node doesn't exist\n");
	}

	// TODO:
	// Check if connection already exists
	// if yes:
	// copy current listeners and add them to the new ones
	// delete previous connection process with sigint or sigterm so that we don't destroy data (no sigkill)
	// create new connection process
	// if no:
	// create new connection process with the new listeners

	// Get sending node's output pipe
	char fifoFrom[256];
	getWritePipeStr(id, fifoFrom);

	// Get listening nodes' input pipes

	// TODO - check that these sizeofs are okei
	char* toPipes[sizeof(listeners)];
	toPipes[sizeof(listeners) - 1] = NULL;

	int j;
	for (j = 0; args[j] != NULL; j++) {

		int nodeToId = (int) strtol(args[j], (char**) NULL, 10);

		printf("(controller) Connecting node %d to %d\n", id, nodeToId);

		// Check that listening process exists
		if (!nodes[nodeToId]) {
			fprintf(stderr, "(controller) Tried to connect a node to a listening node that doesn't exist. Skipping...\n");
			continue;
		}

		char fifoTo[256];
		getReadPipeStr(nodeToId, fifoTo);

		toPipes[j] = fifoTo;
	}

	// Create child to run the connection
	pid_t pid = childCreator_createChild(cmd, args, readPipe, writePipe);

	if (pid < 0) {

		fprintf(stderr, "(controller) Node creation failed\n");
		closeNamedPipePair(nodeId);

		return -1;

	} else {

		nodes[nodeId] = pid;

		return 0;
	}

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

	return 0;
}

/**
 * Creates a node that will run a command with a set of arguments
 *
 * @param cmd The command to be executed by the node
 * @param args The arguments to be applied to the command
 * @return 0 if creation goes fine, -1 if something goes wrong
 */
static int createNode(int nodeId, char* cmd, char** args) {

	printf("Creating node %d with cmd %s and args ", nodeId, cmd);

	int i;
	for (i = 0; args[i] != NULL; i++) {
		printf("%s ", args[i]);
	}

	printf("\n");

	// Create named pipe pair for this node
	if (createNamedPipePair(nodeId) != 0) {
		fprintf(stderr, "Failed to create named pipe\n");
		exit(EXIT_FAILURE);
	}

	// Convert pipeId to string
	char writePipe[256];
	char readPipe[256];
	getWritePipeStr(nodeId, writePipe);
	getReadPipeStr(nodeId, readPipe);

	// Add .exe suffix when in windows environment
	if (WINDOWS_MODE) {

		char windowsCmd[256];
		getWindowsString(cmd, windowsCmd);

		cmd = windowsCmd;
	}

	// Create child to run the node
	pid_t pid = childCreator_createChild(cmd, args, readPipe, writePipe);

	if (pid < 0) {

		fprintf(stderr, "(controller) Node creation failed\n");
		closeNamedPipePair(nodeId);

		return -1;

	} else {

		nodes[nodeId] = pid;

		return 0;
	}
}

/**
 * Spawns a connection process which will connect an inject to a node
 * @param injectId The id of the sending inject
 * @param nodeId The id of the receiving node
 */
static int createInjectConnection(int injectId, int nodeId) {

	// TODO
	// Check that inject pipe exists
	// remove inject connection and simply redirect the output of the inject process to the listening node

	// Get pipe ids

	char injectPipe[256];
	getInjectPipeStr(injectId, injectPipe);

	char pipeTo[256];
	getReadPipeStr(nodeId, pipeTo);

	// Create child to run the inject connection
	pid_t pid = childCreator_createChild(cmd, args, readPipe, writePipe);

	if (pid < 0) {

		fprintf(stderr, "(controller) Inect connection creation failed\n");
		closeNamedPipePair(nodeId);

		return -1;

	} else {

		injectConnections[injectId];

		nodes[nodeId] = pid;

		return 0;
	}

	int pid = fork();

	if (pid != 0) {



	} else {

		// Redirecionar stdout e stderr
		int fd = open("log.txt", O_WRONLY|O_APPEND|O_CREAT, 0666);
		dup2(fd, 1);
		dup2(fd, 2);

		if (WINDOWS_MODE) {

			char* argsToPass[] = {"connect.exe", injectPipe, pipeTo};

			execv("connect.exe", argsToPass);

			fprintf(stderr, "(controller) Connection creation failed (execv(%s, [%s, %s])) Error message: %s\n", "connect.exe", injectPipe, pipeTo, strerror(errno));

		} else {

			char* argsToPass[] = {"connect", injectPipe, pipeTo};

			execv("connect", argsToPass);

			fprintf(stderr, "(controller) Connection creation failed (execv(%s, [%s, %s])) Error message: %s\n", "connect", injectPipe, pipeTo, strerror(errno));
		}

		// Se o execv falhou, terminar com código 127
		exit(127);
	}

	return 0;
}

static int createInject(int nodeId, char* cmd, char** args) {

	printf("Creating inject to node %d with cmd %s and args ", nodeId, cmd);

	int i;
	for (i = 0; args[i] != NULL; i++) {
		printf("%s ", args[i]);
	}

	printf("\n");

	// Criar um filho para correr o nó

	pid_t pid = fork();

	if (pid == 0) {

		// Criar named pipe para este inject

		int injectId = createInjectPipe();

		if (injectId != 0) {
			fprintf(stderr, "Failed to create inject pipe\n");
			exit(EXIT_FAILURE);
		}

		// Redirecionar stderr
		int fd = open("log.txt", O_WRONLY|O_APPEND|O_CREAT, 0666);
		dup2(fd, 2);

		// Converter pipeId para string
		char injectPipe[256];

		getInjectPipeStr(nodeId, injectPipe);

		// Trocar stdout deste processo pela inject pipe

		int res = open(injectPipe, O_WRONLY);

		dup2(res, 1);

		// Adicionar sufixo .exe quando estivermos a trabalhar em windows
		// (isto não é preciso porque estes comandos não devem ser usados mas deixa tar)
		if (WINDOWS_MODE && (
			strcmp(cmd, "filter") == 0 ||
			strcmp(cmd, "const")  == 0 ||
			strcmp(cmd, "window") == 0 ||
			strcmp(cmd, "spawn")  == 0
		)) {
			cmd = strcat(cmd, ".exe");
		}

		// Criar array de argumentos para passar ao exec (primeiro argumento tem de ser o comando)

		char* args2[sizeof(args) + sizeof(char*)];

		args2[0] = cmd;

		int k;
		for (k = 0; args[k] != NULL; k++) {
			args2[k + 1] = args[k];
		}

		// Antes de iniciar o comando, criar o connect deste inject para o nó
		createInjectConnection(injectId, nodeId);

		execv(cmd, args2);

		fprintf(stderr, "(controller) Inject creation failed (execv(%s, [...]))\n", cmd);

		close(res);

		closeInjectPipe(injectId);

		// Se o execv falhou, terminar com código 127
		exit(127);
	}

	return 0;
}



static int removeConnection(int from, int to) {

	if (connections[from][to] != 0) {

		kill(connections[from][to], SIGKILL);

		connections[from][to] = 0;
	}

	return 0;
}

static int removeAllConnections() {

	int i, j;
	for (i = 0; i < MAX_NODES; i++) {

		for (j = 0; j < MAX_NODES; j++) {

			if (connections[i][j] != 0) {

				kill(connections[i][j], SIGKILL);

				connections[i][j] = 0;
			}
		}
	}

	return 0;
}

static int removeAllInjectConnections() {

	int i, j;
	for (i = 0; i < MAX_INJECTS; i++) {

		for (j = 0; j < MAX_NODES; j++) {

			if (injectConnections[i][j] != 0) {

				kill(injectConnections[i][j], SIGKILL);

				injectConnections[i][j] = 0;
			}
		}
	}

	return 0;
}

static int removeNode(int id) {

	if (nodes[id] != 0) {
		kill(nodes[id], SIGKILL);

		nodes[id] = 0;
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

			nodes[i] = 0;
		}

		closeNamedPipePair(i);
	}

	return 0;
}

static int removeAllInjectPipes() {

	int i;
	for (i = 0; i < MAX_INJECTS; i++) {
		if (injectPipes[i] != 0) {
			closeInjectPipe(i);
			injectPipes[i] = 0;
		}
	}

	return 0;
}

void parseCommand(char* cmdLine) {

	char** tokens = split(cmdLine, ' ');

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

		createInject(id, tokens[2], &tokens[3]);

	} else {

		fprintf(stderr, "Skipping unknown command in configuration file (%s)\n", cmd);
	}
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
	memset(injectPipes, 0, sizeof(injectPipes));
	memset(injectConnections, 0, sizeof(injectConnections[0][0]) * MAX_INJECTS * MAX_NODES);

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

				parseCommand(buffer);
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
			removeAllInjectConnections();
			removeAllInjectPipes();

		} else {

			parseCommand(buffer2);
		}
	}

	if (j < 0) {
		fprintf(stderr, "Error reading user input: %s (readLine returned %d)\n", strerror(errno), j);
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}
