CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -g -D_DEFAULT_SOURCE

all:
	$(CC) $(CFLAGS) src/main.c -o planificador

clean:
	rm -f planificador
