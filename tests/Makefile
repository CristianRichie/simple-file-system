DINCLUDE=../include
DSRC=../src

CC=gcc
CFLAGS= -Wall -g -std=gnu99 -Wstrict-prototypes -I$(DINCLUDE)

SRC=$(wildcard *.c)
BINS=$(SRC:.c=)
OBJ=$(wildcard $(DSRC)/*.o)

.phony: clean all tests

all: $(BINS)

%: %.c 
	$(CC) -o $@ $< $(OBJ) $(CFLAGS)

run:
	./run_tests.sh

clean:
	rm -rf $(BINS) *.img vgcore*
