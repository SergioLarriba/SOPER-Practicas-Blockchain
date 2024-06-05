CC=gcc
CFLAGS= -g -Wall -pedantic -Wextra
EJS = miner monitor
CLIBS = -lrt -pthread 
########################################################
OBJECTS = pow.o miner.o monitor.o
########################################################

all: $(EJS) 

miner: miner.o pow.o monitor.o 
	$(CC) $(CFLAGS) -o miner miner.o pow.o $(OBJ) $(CLIBS)

monitor: monitor.o pow.o miner.o 
	$(CC) $(CFLAGS) -o monitor monitor.o pow.o $(OBJ) $(CLIBS)
	
miner.o: miner.c pow.h
	$(CC) $(CFLAGS) -c miner.c

monitor.o: monitor.c pow.h
	$(CC) $(CFLAGS) -c monitor.c
	
pow.o: pow.c pow.h
	$(CC) $(CFLAGS) -c pow.c
	
.PHONY: clean

clean:
	rm -rf *.o miner monitor