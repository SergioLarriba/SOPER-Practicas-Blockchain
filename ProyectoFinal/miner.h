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
#define MAX_MINERS 100
#define SHM_NAME "/shm_file"
#define N_TIMES 50

typedef struct
{
    long objetivo;
    long solucion;
    int valido;
} Message; // Lo que contiene cada bloque

typedef struct
{
    pid_t pid;
    bool vote;
    long n_coins;
} MinerInfo;

typedef struct
{
    Message message[MAX_NUM_MESSAGES]; // Tamaño maximo de 7 en la cola y memoria
    sem_t sem_empty;
    sem_t sem_mutex;
    sem_t sem_fill;
    sem_t sem_minerInfo;
    MinerInfo minerInfo[MAX_MINERS];
    int n_miners;
    long target;
    long solution;
    pid_t winner;
    int votes_yes;
    int votes_no;
} MemoriaCompartida;

#endif