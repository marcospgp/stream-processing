#ifndef GLOBALS_H
#define GLOBALS_H

#define WINDOWS_MODE 1
#define FIFO_PREFIX "/tmp/sofifo/"
#define MAX_NODES 2048

void getWritePipeStr(int pipeId, char* buffer);

void getReadPipeStr(int pipeId, char* buffer);

// Duplica uma string
char* duplicateString(char *org);

/* Retorna o indíce do caractere onde começa um elemento
 * numa string, assumindo que o formato da string é:
 *
 *     string = "elemento1:elemento2:elemento3:elemento4"
 *
 * Neste caso, o resultado de getIndexOfElement(string, 2) é 10
 */
long getIndexOfElement(char* string, long n);

/* Obtém o valor de um determinado elemento da string, até ao tamanho máximo bufferSize (em bytes)
 * Retorna 0 se houve algum problema, número de bytes lidos se correu normalmente
 *
 * Exemplo:
 *     getElementValue("elemento1:elemento2:elemento3:elemento4", 3, buffer);
 * Retorna 9 e buffer == "elemento3"
 */
long getElementValue(char* string, long n, char* buffer, long bufferSize);

// Array dinâmico

#define INITIAL_ARRAY_SIZE 1024

typedef struct {
	int *array;
	size_t used;
	size_t size;
} Array;

void initArray(Array *a);
void insertArray(Array *a, int element);
void freeArray(Array *a);

// Divide uma string em várias com base num delimitador. O array resultante termina em NULL
char** split(char* a_str, const char a_delim);

#endif
