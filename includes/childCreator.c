#include "childCreator.h"

// Check childCreator.h for documentation

pid_t childCreator_createChild(char* cmd, char** args, char* readPipe, char* writePipe) {

	pid_t pid = fork();

	if (pid != 0) {

		// Parent returns child pid
		return pid;

	} else {

		// Child runs cmd

		// Redirect stderr
		int fd = open("log.txt", O_WRONLY|O_APPEND|O_CREAT, 0666);
		dup2(fd, 2);

		// Switch stdout and stdin of this process for named pipes
		// (if pipes were passed)

		int resR, resW;

		if (readPipe != NULL) {
			resR = open(readPipe, O_RDONLY);
			dup2(resR, 0);
		}

		if (writePipe != NULL) {
			resW = open(writePipe, O_WRONLY);
			dup2(resW, 1);
		}

		// Recreate args array with cmd as first argument to pass to exec

		char* args2[sizeof(args) + 1];

		args2[0] = cmd;

		int k;
		for (k = 0; args[k] != NULL; k++) {
			args2[k + 1] = args[k];
		}

		args2[k + 1] = NULL;

		execv(cmd, args2);

		fprintf(stderr, "(childCreator) Node creation failed (execv(%s, [...]))\n", cmd);

		closeNamedPipePair(nodeId);

		close(fd);

		if (readPipe != NULL) {
			close(resR);
		}

		if (writePipe != NULL) {
			close(resW);
		}

		// If execv failed, exit with code 127
		exit(127);
	}
}
