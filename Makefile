NAME=extract
MAIN=main

SRC_DIR=./src
BIN_DIR=./bin

SRC=$(SRC_DIR)/$(NAME).c
BIN=$(BIN_DIR)/$(NAME)

# --------

CC=clang
CFLAGS=-Wall -Wextra -pedantic

RM=rm

# --------

all: $(BIN)

clean:
	$(RM) ./$(BIN) || true

$(BIN): $(SRC)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^


