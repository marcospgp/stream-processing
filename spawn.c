#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "includes/globals.h"
#include "includes/readLine.h"

int main(int argc, char** argv) {

	if (argc < 2) {
		return EXIT_FAILURE;
	}

	/* array para onde se vai ler os argumentos que tem $ na frente */
	long input[128];

	char* end;
	long read;

	/* array que guarda o i de argv[i] onde os $ vao sendo encontrados */
	int* positions = malloc(sizeof(int)*1);

	/* conta o numero de argumentos que leu */
	int nread = 0;


	/* ciclo que que le os argumentos com $ na frente e os passa para um array input. O i do argv[i]
	fica guardado no array positions */

	for (int i = 1; i < argc; i++) {
	 	if (argv[i][0] == '$') {
	 		read = strtol(&(argv[i][1]), &end, 10);
	 		if (read == 0 && errno == 0) {
				fprintf(stderr, "Error reading arguments: %s\n", strerror(errno));
			}
			else {
	 			input[nread] = read;
	 			*(positions+nread) = i;
	 			nread++;
	 			positions = realloc(positions,sizeof(int)*1);
	 		}
	 	}
	 }

	// Ler linhas do stdin

	char buffer[PIPE_BUF];
	char* columnAValue = malloc(PIPE_BUF);
	ssize_t i;

	while ( (i = readLine(0, buffer, (long) PIPE_BUF)) > 0) {

		int counter = 0;

		// Ciclo que obtem os valores das colunas que o argumento da funcao manda ler

		for(int i = 0; i < nread; i++) {
			if (getElementValue(buffer, input[i], &columnAValue[i], (long) PIPE_BUF) < 1) {
				fprintf(stderr, "(filter) Error obtaining column values (getElementValue returned 0)");
				return EXIT_FAILURE;
			}
			else {
				strcpy(argv[*(positions+counter)], &columnAValue[i]); // copia o valor que esta na coluna para o argv
				counter++;
			}
		}

		char *output = malloc(PIPE_BUF);

		int exitStatus;
		int result;
		pid_t childId = fork(); // cria 1 processo filho

		if (childId == 0) {
			execvp(argv[1], &argv[1]); // executa os argumentos ja manipulados
			exit(0);
		}

		while (wait(&exitStatus) > 0) {
			result = WEXITSTATUS(exitStatus);
		}


		/* coloca o resultado do exec, p.e. --> input: a:3:x:2, output a:3:x:2:0 */

		sprintf(output, "%d", result);

		buffer[i - 1] = ':';
		buffer[i] = '\0';

		strcat(buffer, output);

		ssize_t newLength = i + strlen(output);

		buffer[newLength] = '\n';
		buffer[newLength + 1] = '\0';

		newLength++;

		write(1, buffer, newLength);
		

	}

	if (i < 0) {

		fprintf(stderr, "(filter) Error reading input: %s (read() returned %ld)\n", strerror(errno), i);
		return EXIT_FAILURE;

	} else {

		return EXIT_SUCCESS;
	}
	
}