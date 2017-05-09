CC = gcc
CCFLAGS = -Wall -std=c11 -g
LIBS =
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
NAMES = $(SOURCES:.c=)
INCLUDES = $(wildcard includes/*.c)
INCLUDESOBJ = $(INCLUDES:.c=.o)
TARGET = program

# http://stackoverflow.com/a/22735335/2037431
.DEFAULT_GOAL := all

# Esta regra recebe qualquer argumento e compila todos os .c dentro da pasta includes,
# depois compila o argumento juntamente com todos os includes (mesmo os desnecessários)

%: %.o $(INCLUDESOBJ)
	$(CC) -o $@ $^ $(LIBS)

# Esta regra compila todos os .c desta pasta juntamente com todos os ficheiros da pasta include,
# mas nunca mistura dois .c desta pasta (porque supostamente todos os .c nesta pasta têm um main)

all: $(NAMES)

# Esta regra é a que usamos no projeto wikipedia-metadata-parser

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

# Regras auxiliares

%.o: %.c %.h
	$(CC) $(CCFLAGS) -o $@ -c $< $(LIBS)

%.o: %.c
	$(CC) $(CCFLAGS) -o $@ -c $< $(LIBS)

clean:
	rm -f *.o *.stackdump *.exe $(TARGET)

.PHONY: all clean # The .PHONY rule keeps make from doing something with a file named all, clean, etc.
