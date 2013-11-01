CFLAGS ?= -std=gnu99 -O0 -g3 -pedantic -Wall -Wextra
CC ?= gcc
BIN = bin2c

$(BIN): $(BIN).c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BIN)

.PHONY: clean
