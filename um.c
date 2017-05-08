#include <stdio.h>
#include <unistd.h> /* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h> /* O_RDONLY, O_WRONLY, O_CREAT, O_* */
#include <stdlib.h>
#include <string.h>

/* int open(const char *path, int oflag [, mode]);
ssize_t read(int fildes, void *buf, size_t nbyte);
ssize_t write(int fildes, const void *buf, size_t nbyte);
int close(int fildes); */


// argv[1] eh o 10
// minibuf e a:3:x:4

int main (int argc, char **argv) {

	char *input = argv[1];
	char *minibuf = malloc(sizeof(char)*7);

	int n = read(0, minibuf, 7);
	strcat(minibuf,":");
	strcat(minibuf, input);

	write(1, minibuf, strlen(minibuf));

	return 0;

}

