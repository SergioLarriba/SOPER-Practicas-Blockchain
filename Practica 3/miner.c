/**
 * @file miningSystem.c
 * @authors Miguel Lozano y Sergio Larriba
 * @brief Implements a mining system to run a proof of work.
 * @date 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <mqueue.h>

#include "pow.h"
#include "monitor.h"
#include "miner.h"

#define BUFF_SIZE 64
#define MQ_NAME "/MyMessageQueue"

/* Global variables */
Message message; 

int main(int argc, char** argv) {
    int rondas, i=0, nbytes=0; 
    double lag; 
    long objetivo_inicial, solucion;  
    char buffer[BUFF_SIZE]; 
    mqd_t mqdes; 
    FILE *f=NULL;  

    //Compruebo argumentos 
    if (argc != 3) {
        printf ("ERROR, ./miner <ROUNDS> <LAG>\n");  
        return(EXIT_FAILURE); 
    }
    //Asigno valores a los argumentos 
    rondas = atoi(argv[1]); 
    lag = atoi(argv[2]); 
    //Estructura cola mensajes
    struct mq_attr mq_struct = {
        .mq_flags = 0,
        .mq_maxmsg = MAX_NUM_MESSAGES,
        .mq_msgsize = sizeof(Message),
        .mq_curmsgs = 0,
    }; 
    //mq_open
    mqdes = mq_open(MQ_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &mq_struct); 
    if (mqdes == -1) {
        perror("mq_open"); 
        exit(EXIT_FAILURE); 
    }
    //Abro fichero 
    f = fopen("SecuenciaGenerada.txt", "r"); 
    if (f == NULL) {
        printf("No se pudo abrir el archivo\n");
        return 1;
    }
    //Generating blocks 
    fprintf(stdout, "[%d] Generating blocks...\n", getpid()); 
    //Leo el fichero y voy viendo los objetivos
    fgets(buffer, BUFF_SIZE, f); 
    objetivo_inicial = strtol(buffer, NULL, 10); 
    while ((fscanf(f, "%ld\n", &solucion) == 1) && i<rondas) {
        message.objetivo = objetivo_inicial; 
        message.solucion = solucion; 
        message.valido = 0; 
        nbytes = mq_send(mqdes,(char*) &message, sizeof(Message), 0); 
        if (nbytes == -1) {
            perror("mq_send"); 
            exit(EXIT_FAILURE); 
        }
        objetivo_inicial = solucion; 
        i++; 
        usleep(lag * 1000); 
    }
    // Establezco el bloque fin porque ya he acabado todas las rondas 
    message.solucion = POW_LIMIT+100; 
    nbytes = mq_send(mqdes,(char*) &message, sizeof(Message), 0); 
    if (nbytes == -1) {
        perror("mq_send");
        exit(EXIT_FAILURE);  
    }
    //Finished generating blocks 
    fprintf(stdout, "[%d] Finishing\n", getpid()); 

    mq_close(mqdes); 
    mq_unlink(MQ_NAME); 
    fclose(f); 

    return 0; 
}

