CC=gcc
CFLAGS= -g -Wall -pedantic -Wextra
EJS = miner monitor
CLIBS = -lrt -pthread 
########################################################
OBJECTS = pow.o miner.o monitor.o
########################################################

all: $(EJS) 

miner: miner.o pow.o monitor.o  registrador.o
	$(CC) $(CFLAGS) -o miner miner.o pow.o registrador.o $(OBJ) $(CLIBS)

monitor: monitor.o pow.o miner.o 
	$(CC) $(CFLAGS) -o monitor monitor.o pow.o $(OBJ) $(CLIBS)
	
miner.o: miner.c pow.h registrador.h miner.h
	$(CC) $(CFLAGS) -c miner.c

monitor.o: monitor.c monitor.h pow.h miner.h
	$(CC) $(CFLAGS) -c monitor.c

registrador.o: registrador.c pow.h registrador.h
	$(CC) $(CFLAGS) -c registrador.c	

pow.o: pow.c pow.h
	$(CC) $(CFLAGS) -c pow.c
	
.PHONY: clean

clean:
	rm -rf *.o *.dat miner monitor

