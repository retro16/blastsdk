CFLAGS ?= -std=gnu99 -O0 -g3 -pedantic -Wall -Wextra
CC ?= gcc
BIN = bin2c

INSTALL_DIR ?= $(HOME)/bin

$(BIN): $(BIN).c
	$(CC) $(CFLAGS) -o $@ $<

install: $(BIN)
	cp $(BIN) $(INSTALL_DIR)

clean:
	rm -f $(BIN)

.PHONY: install clean
