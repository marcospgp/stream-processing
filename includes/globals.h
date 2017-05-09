#ifndef GLOBALS_H
#define GLOBALS_H

#define PIPE_BUF 1024

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

#endif
