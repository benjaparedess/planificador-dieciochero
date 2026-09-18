CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -g
LDFLAGS = -lpthread

SRC = src/main.c
BIN = planificador

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(BIN)
