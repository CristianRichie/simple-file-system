DINCLUDE=./include
DSRC=./src

CC=gcc
CFLAGS=-Wall -g -std=gnu99 -Wstrict-prototypes -I$(DINCLUDE)

HEADERS=$(wildcard $(DINCLUDE)/*)
SRC=$(wildcard $(DSRC)/*.c)
OBJ=$(patsubst %.c,%.o,$(wildcard $(DSRC)/*.c))

.PHONY: clean all

all: $(OBJ) test

%.o: %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

run:
	make -s -C ./tests run

test:
	make -C ./tests

clean:
	make -C ./tests clean
	rm -rf $(DSRC)/*.o
