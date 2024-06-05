#ifndef MINER_H 
#define MINER_H 

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>
#include <mqueue.h>
#include <string.h>

#define BUFF_SIZE 64
#define MAX_NUM_MESSAGES 7 

/* Global variables */
int work_per_thread, n_threads_global;
long int target, next_target, thread_result;

typedef struct {
    long objetivo; 
    long solucion; 
    int valido; 
} Message; //Lo que contiene cada bloque 

typedef struct {
    Message message[7]; //Tamaño maximo de 7 en la cola y memoria 
    sem_t sem_empty; 
    sem_t sem_mutex; 
    sem_t sem_fill; 
} MemoriaCompartida; 

/**
 * @brief Iterate the pow_hash function for different inputs.
 * 
 * @param arg The id associated to a thread.
*/
void *miner(void *arg);

#endif 