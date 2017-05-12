CC = gcc
CCFLAGS = -O2 -Wall -g
LIBS =
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
TARGETS = $(SOURCES:.c=)
INCLUDES = $(wildcard includes/*.c)
INCLUDESOBJ = $(INCLUDES:.c=.o)

all: $(TARGETS)

$(TARGETS): $(OBJECTS) $(INCLUDESOBJ)
	$(CC) -o $@ $@.o $(INCLUDESOBJ) $(LIBS)

%.o: %.c %.h
	$(CC) $(CCFLAGS) -c $< -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@ $(LIBS)

clean:
	rm -f *.o *.stackdump *.exe

.PHONY: all clean # The .PHONY rule keeps make from doing something with a file named clean
