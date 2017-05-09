#ifndef READLINE_H
#define READLINE_H

#define PIPE_BUF 1024

/* Retorna o indíce do caractere onde começa um elemento
 * numa string, assumindo que o formato da string é:
 *
 * string = "elemento1:elemento2:elemento3:elemento4"
 *
 * Neste caso, o resultado de getIndexOfElement(string, 2) é 10
 */
long getIndexOfElement(char* string, long n);

#endif
