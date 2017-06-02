#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "includes/globals.h"
#include "includes/readLine.h"
#include "includes/childCreator.h"

static int nodes[MAX_NODES]; // nodes[nodeId] = processID (0 if it doesn't exist)

// Connection proceses - each node has one that copies its output as many times as needed
// and sends it to all the listening nodes
static int connections[MAX_NODES]; // connections[from] = processID (0 if it doesn't exist)

// The connectionDests array keeps track of what nodes are listening to the output of each node
static int connectionDests[MAX_NODES][MAX_OUTGOING_CONNECTIONS]; // connectionDests[nodeId] = listeningNodeIds[] (array ends at value -1)

// We need the injects array to keep track of the available ids for injects and their pipes
static int injects[MAX_INJECTS]; // injects[injectId] = 1 if inject exists, 0 if it doesn't

// Inject connection processes - each inject process has one that sends its output to one listening node
static int injectConnections[MAX_INJECTS]; // injectConnections[injId] = processID (0 if it doesn't exist)

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
static void closeInjectPipe(int injectId) {

	char pipe[256];

	getInjectPipeStr(injectId, pipe);

	// Check that pipe exists
	if (access(pipe, F_OK) != -1) {
		unlink(pipe);
	}

	// Signal that this id is now available
	injects[injectId] = 0;

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
 * @return  The new inject's id or -1 if something went wrong
 */
static int createInjectPipe() {

	// Find available pipe id
	int i, foundFree = 0;
	for (i = 0; i < MAX_INJECTS; i++) {
		if (injects[i] == 0) {
			foundFree = 1;
			injects[i] = 1;
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
 * @param listeners A string array containing the id's of the listening nodes.
                    if this is NULL, createConnection tries to use the listeners
                    present in connectionDests
 * @return 0 on success, -1 if something goes wrong
 */
static int createConnection(int id, char** listeners) {

	// Check if connection already exists
	if (connections[id]) {

		printf("(controller) Connection process detected while creating new connection. About to kill previous process and waitpid()\n");

		kill(connections[id], SIGTERM); // SIGTERM so that we don't lose data
		waitpid(connections[id], NULL, 0); // Wait for process to close
	}

	// Create allListeners array to be sent to connection process
	char* allListeners[MAX_OUTGOING_CONNECTIONS];

	int k;
	for (k = 0; connectionDests[id][k] != -1; k++) {

		// Convert id to string
		char str[64];
		sprintf(str, "%d", connectionDests[id][k]);

		allListeners[k] = str;
	}

	int destsSize = k;

	if (listeners != NULL) {

		// Add new listeners to connectionDests and allListeners arrays
		int i;
		for (i = 0; listeners[i] != NULL; i++) {

			connectionDests[id][destsSize] = (int) strtol(listeners[i], (char**) NULL, 10);

			char str[64];
			sprintf(str, "%d", connectionDests[id][destsSize]);

			allListeners[destsSize] = str;

			destsSize++;
		}
	}

	connectionDests[id][destsSize] = -1;
	allListeners[destsSize] = NULL;

	// Get sending node's output pipe
	char fifoFrom[128];
	getWritePipeStr(id, fifoFrom);

	char *cmd = "connect";

	// Add .exe suffix when in windows environment
	if (WINDOWS_MODE) {

		char windowsCmd[128];
		getWindowsString(cmd, windowsCmd);

		cmd = windowsCmd;
	}

	// Create child to run the connection
	// Use allListeners to tell process what nodes to send the output to
	pid_t pid = childCreator_createChild(cmd, allListeners, fifoFrom, NULL);

	if (pid < 0) {

		fprintf(stderr, "(controller) Connection creation failed\n");
		connections[id] = 0;

		return -1;

	} else {

		connections[id] = pid;

		return 0;
	}
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
 *
 * @param injectId The id of the sending inject
 * @param nodeId The id of the receiving node
 * @return 0 on success, -1 if something went wrong
 */
static int createInjectConnection(int injectId, int nodeId) {

	// Get pipe ids

	char injectPipe[256];
	getInjectPipeStr(injectId, injectPipe);

	char pipeTo[256];
	getReadPipeStr(nodeId, pipeTo);

	char *cmd = "connect";

	// Add .exe suffix when in windows environment
	if (WINDOWS_MODE) {

		char windowsCmd[128];
		getWindowsString(cmd, windowsCmd);

		cmd = windowsCmd;
	}

	// Create child to run the inject connection
	// (Sending no arguments to connect makes it use the writePipe as output)
	pid_t pid = childCreator_createChild(cmd, NULL, injectPipe, pipeTo);

	if (pid < 0) {

		fprintf(stderr, "(controller) Inect connection creation failed\n");

		return -1;

	} else {

		injectConnections[injectId] = pid;

		return 0;
	}
}

static int createInject(int nodeId, char* cmd, char** args) {

	printf("Creating inject to node %d with cmd %s and args ", nodeId, cmd);

	int i;
	for (i = 0; args[i] != NULL; i++) {
		printf("%s ", args[i]);
	}

	printf("\n");

	// Criar named pipe para este inject

	int injectId = createInjectPipe();

	if (injectId != 0) {
		fprintf(stderr, "Failed to create inject pipe\n");
		exit(EXIT_FAILURE);
	}

	// Converter pipeId para string
	char injectPipe[128];

	getInjectPipeStr(nodeId, injectPipe);

	// Criar o connect deste inject para o nó
	createInjectConnection(injectId, nodeId);

	// Add .exe suffix when in windows environment
	if (WINDOWS_MODE) {

		char windowsCmd[128];
		getWindowsString(cmd, windowsCmd);

		cmd = windowsCmd;
	}

	// Criar um filho para correr o inject
	pid_t pid = childCreator_createChild(cmd, args, NULL, injectPipe);

	if (pid < 0) {

		fprintf(stderr, "(controller) Inject creation failed\n");

		closeInjectPipe(injectId);

		return -1;

	} else {

		injects[injectId] = pid;

		return 0;
	}
}

static int removeConnection(int id) {

	if (connections[id] != 0) {

		kill(connections[id], SIGTERM);

		connections[id] = 0;
		connectionDests[id][0] = -1;
	}

	return 0;
}

static int removeAllConnections() {

	int i;
	for (i = 0; i < MAX_NODES; i++) {

		removeConnection(i);
	}

	return 0;
}

static int removeAllInjectConnections() {

	int i;
	for (i = 0; i < MAX_INJECTS; i++) {

		if (injectConnections[i] != 0) {

			kill(injectConnections[i], SIGTERM);

			injectConnections[i] = 0;
		}

		// Remove inject pipe too
		if (injects[i] != 0) {

			closeInjectPipe(i);

			// And signal that the id is now free
			injects[i]= 0;
		}
	}

	return 0;
}

static int removeNode(int id) {

	if (nodes[id] != 0) {

		kill(nodes[id], SIGTERM);

		nodes[id] = 0;
	}

	closeNamedPipePair(id);

	removeConnection(id);

	return 0;
}

static int removeAllNodes() {

	int i;
	for (i = 0; i < MAX_NODES; i++) {

		removeNode(i);
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

		// Remove "to" from connectionDests
		int i, found = 0;
		for (i = 0; connectionDests[id][i] != -1; i++) {

			if (!found && connectionDests[id][i] == to) {

				found = 1;

			} else if (found) {

				connectionDests[id][i - 1] = connectionDests[id][i];
			}
		}

		createConnection(id, NULL);

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

	// Inicializar arrays a 0
	memset(nodes, 0, sizeof(nodes));
	memset(connections, 0, sizeof(connections));
	memset(injects, 0, sizeof(injects));
	memset(injectConnections, 0, sizeof(injectConnections));

	// Inicializar array connectionDests a -1
	// (apenas o primeiro valor, para sinalizar que o array acaba ali)
	int k;
	for (k = 0; k < MAX_NODES; k++) {
		connectionDests[k][0] = -1;
	}

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
