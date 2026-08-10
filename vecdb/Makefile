CC      = gcc
CFLAGS  = -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -O2 -g
LDFLAGS = -pthread -lm -lreadline -lhistory -ltermcap
SRC     = $(wildcard src/*.c)
OBJ     = $(SRC:.c=.o)
BIN     = vecdb

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(BIN) $(OBJ)

.PHONY: all clean
