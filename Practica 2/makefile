# makefile for practice 2

EXE = voting

SRC = voting.c voter.c

OBJ = $(SRC:%.c=%.o)


CFLAGS = -std=c11 -g -Wall -Wpedantic -Wextra

CLIBS = -lpthread

all: $(EXE)

$(EXE): $(OBJ)
	gcc $(CFLAGS) -o $(EXE) $(OBJ) $(CLIBS)


%.o: %.c
	gcc $(CFLAGS) -c $< $(CLIBS)

# Dependencies
voting.o: voting.c voter.h names.h
voter.o: voter.c voter.h names.h


.PHONY: clean
clean:
	rm -f $(EXE) $(OBJ)

clear:
	rm -f $(OBJ)