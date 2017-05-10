#include <stdio.h>
#include <unistd.h> /* chamadas ao sistema: defs e decls essenciais */
#include <stdlib.h>
#include <string.h>
#include "includes/globals.h"
#include "includes/readLine.h"

/* int open(const char *path, int oflag [, mode]);
ssize_t read(int fildes, void *buf, size_t nbyte);
ssize_t write(int fildes, const void *buf, size_t nbyte);
int close(int fildes); */


// argv[1] eh o 10
// minibuf e a:3:x:4

int main (int argc, char **argv) {

	char *input = argv[1];
	char *minibuf = malloc(PIPE_BUF);

	readLine(0, minibuf, PIPE_BUF);

	int i = strlen(minibuf);
    strcpy(&minibuf[i-1],":");
    strcat(minibuf,input);
    strcat(minibuf,"\n");

	write(1, minibuf, strlen(minibuf));

	return 0;

}
