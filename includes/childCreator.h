#ifndef CHILDCREATOR_H
#define CHILDCREATOR_H

/**
 * Create a child process that runs a certain command with the given arguments.
 * If readPipe or writePipe are not null, the stdin or stdout of the child process
 * will be replaced accordingly.
 *
 * @param cmd The command to run
 * @param args The arguments to run with the command (these should NOT start with the command itself,
 *             as this function does that for you)
 * @param readPipe Named pipe to replace stdin with if not NULL
 * @param writePipe Named pipe to replace stdout with if not NULL
 * @return The pid of the child process on success, -1 if something goes wrong
 */
pid_t childCreator_createChild(char* cmd, char** args, char* readPipe, char* writePipe);

#endif
