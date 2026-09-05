CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -std=gnu99 -I../../include

BIN = hidpadtest

.PHONY: all clean rebuild test

all: $(BIN)

$(BIN): hidpadtest.c
	$(CC) $(CFLAGS) -o $@ $^

test: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN) *.o

rebuild: clean all