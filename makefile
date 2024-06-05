# makefile for practice 1
# author: Miguel Lozano

EXE = mrush

SRC = mrush.c pow.c miningSystem.c monitor.c

OBJ = $(SRC:%.c=%.o)

CFLAGS = -g -Wall -Wextra -ansi -pedantic -O2

CLIBS = -pthread

all: $(EXE)

$(EXE): $(OBJ)
	@echo -e "\nLINKING OBJECT FILES"
	gcc $(CFLAGS) -o $(EXE) $(OBJ) $(CLIBS)


# Automatise the compiling of all the source files
%.o: %.c
	@echo -e "\nCOMPILING $<"
	gcc $(CFLAGS) -c $<

# Filled with the dependencies of every object file
mrush.o: mrush.c pow.h miningSystem.h
pow.o: pow.c pow.h miningSystem.h
miningSystem.o: miningSystem.c miningSystem.h pow.h monitor.h
monitor.o: monitor.c miningSystem.h pow.h monitor.h

.PHONY: clean clear
clean:
	@echo -e "\n----- REMOVING EXECUTABLE AND OBJECT FILES -----"
	rm -f $(EXE) $(OBJ)

clear:
	@echo -e "\n----- REMOVING OBJECT FILES -----"
	rm -f $(OBJ)