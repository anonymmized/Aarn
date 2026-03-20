CC ?= cc
CFLAGS ?= -Wall -Wextra -Wpedantic -std=c11
DBGFLAGS ?= -g -fsanitize=address,undefined

SRC := $(wildcard src/*.c) $(wildcard src/preview/*.c) $(wildcard helps/*.c)
BIN := bin/aarn

.PHONY: all debug clean

all: $(BIN)

$(BIN): $(SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

debug:
	@mkdir -p bin
	$(CC) $(CFLAGS) $(DBGFLAGS) $(SRC) -o bin/aarn_debug

clean:
	rm -f $(BIN) bin/aarn_debug
