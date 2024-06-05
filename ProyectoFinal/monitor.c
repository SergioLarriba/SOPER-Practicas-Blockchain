#include <fcntl.h>
#include <mqueue.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>

#include "monitor.h"
#include "miner.h"
#include "pow.h"

#define MQ_NAME "/MyMessageQueue"
#define BUF_SIZE 150
#define MAX_TAM_BUFFER_CIRCULAR 7

void comprobador(int tiempo,int fd);
void monitor(int tiempo, int fd);
MemoriaCompartida memoriaCompartida;

int llamada_a_Pow(MemoriaCompartida m) {
    if(pow_hash(m.message->solucion)==m.message->objetivo){
        return 1;
    }
    return -1;
}

int main(int argc,char *argv[]) {
    int lag=0;
    int fd; 
    // Compruebo argumentos 
    if (argc != 2) {
        fprintf(stderr, "Por favor, ingrese bien los argumentos: ./monitor <LAG> \n");
        return EXIT_FAILURE;
    }

    lag = atoi(argv[1]);

    // Creo la memoriaCompartida compartida, si existe voy a comprobador 
    if ((fd = shm_open(SHM_NAME, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR ))==-1) {
        // Si existe, se ejecuta el proceso monitor 
        if (errno == EEXIST) {
            monitor(lag, fd); 
        } else {
            perror("shm_open"); 
            exit(EXIT_FAILURE); 
        }
    } else { // Comprobador 
        comprobador(lag, fd); 
    }
    return EXIT_SUCCESS; 
}


void comprobador(int lag,int fd){
    int i=0;
    mqd_t mqdes;
    MemoriaCompartida *shm; 
    MemoriaCompartida mess[MAX_TAM_BUFFER_CIRCULAR]; //para leer la memoriaCompartida compartida con un bufer circular de 6  

    //Especifico el tamaño de la memoria compartida 
    ftruncate(fd,sizeof(MemoriaCompartida));
    //Asigno una region de memoria virtual 
    shm = mmap(NULL, sizeof(MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap"); 
        exit(EXIT_FAILURE); 
    }
    //Inicializo los semaforos en memoria compartida 
    if(sem_init(&(shm->sem_empty) ,1,6)==-1){
        perror("ERROR CREANDO SEM_EMPTY");
        exit(EXIT_FAILURE);
    }
    if(sem_init(&(shm->sem_fill),1,0)==-1){
        perror("ERROR CREANDO SEM_FILL");
        exit(EXIT_FAILURE);
    }
    if(sem_init(&(shm->sem_mutex),1,1)==-1){
        perror("ERROR CREANDO SEM_MUTEX");
        exit(EXIT_FAILURE);
    }
    //Abro la cola de mensajes 
    mqdes=mq_open(MQ_NAME,O_RDONLY);
    if (mqdes == ((mqd_t)-1)) {
        perror("mq_open"); 
        exit(EXIT_FAILURE); 
    }
    printf ("[%d] Checking blocks...\n", getpid()); 
    for(i=0;true;i++){
        i=i%6;
        //Recojo los mensajes y los guardo en una estructura auxiliar 
        if(mq_receive(mqdes,(char*)&mess[i].message, sizeof(Message),NULL)==-1){
            perror("mq_receive");
            exit(1);
        } 
        //Bajo semaforos 
        sem_wait(&(shm->sem_empty));
        sem_wait(&(shm->sem_mutex));
        //Lo guardo en mi estructura de memoria compartida 
        memoriaCompartida.message[i]=*mess[i].message;
        shm->message[i].valido=1;
        shm->message[i].objetivo=memoriaCompartida.message[i].objetivo; 
        shm->message[i].solucion=memoriaCompartida.message[i].solucion; 
        //printf ("Objetivo: %ld, Solucion: %ld\n", shm->message[i].objetivo, shm->message[i].solucion); 
        //Si la solucion está correcta 
        if(llamada_a_Pow(mess[i]) == 1){
            memoriaCompartida.message[i].valido = 1; 
        } else {
            memoriaCompartida.message[i].valido = 0; 
        }
        if (memoriaCompartida.message[i].valido==1 && shm->message[i].solucion<POW_LIMIT) {
            //Lo escribo en memoria compartida 
            if(memcpy(shm, &memoriaCompartida.message[i],sizeof(Message))==NULL){
                perror("memcpy");
                exit(EXIT_FAILURE);
            }
        }
        //Subo semaforos 
        sem_post(&(shm->sem_mutex));
        sem_post(&(shm->sem_fill));
        //El bloque de finalizacion es mayor que POW_LIMIT 
        if(shm->message[i].solucion>POW_LIMIT){
            printf("[%d] Finishing\n", getpid());
            fflush(stdout);
            //Libero recursos 
            munmap(shm, sizeof(MemoriaCompartida));
            shm_unlink(SHM_NAME);
            sem_destroy(&(shm->sem_empty));
            sem_destroy(&(shm->sem_fill));
            sem_destroy(&(shm->sem_mutex));
            mq_close(mqdes);
            mq_unlink(MQ_NAME);
            exit(EXIT_SUCCESS);
        }
        usleep(1000*lag);
    }
}


void monitor(int lag, int fd) { 
    MemoriaCompartida *shm; 
    int i; 
    // Abro la memoriaCompartidaCompartida compartida en modo lectura 
    fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (fd == -1) {
        perror("shm_open"); 
        exit(EXIT_FAILURE); 
    } 
    //Extraigo el bloque (Leo)
    shm = mmap(NULL, sizeof(MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    if (shm == MAP_FAILED) { 
        perror("mmap"); 
        exit(EXIT_FAILURE); 
    }
    printf ("[%d] Printing blocks...\n", getpid()); 
    fflush(stdout);
    // Repito el ciclo extraccion y muestra hasta recibir un bloque de finalizacion
    for(i=0; true;) {
        i=i%6; 
        //Bajo semaforos 
        sem_wait(&(shm->sem_fill)); 
        sem_wait(&(shm->sem_mutex)); 
        //Muestro el bloque por pantalla 
        if (shm->message[i].valido==1 && shm->message[i].solucion < POW_LIMIT) {
            printf("Solution accepted: %08ld --> %08ld\n", shm->message[i].objetivo, shm->message[i].solucion); 
            fflush(stdout);
        } else if (shm->message[i].valido==0 && shm->message[i].solucion < POW_LIMIT) {
            printf("Solution rejected: %08ld !-> %08ld\n", shm->message[i].objetivo, shm->message[i].solucion); 
            fflush(stdout);
        }
        //Si me llega el bloque fin (aquel que sea mayor que el pow limit)
        if (shm->message[i].solucion > POW_LIMIT) {
            printf("[%d] Finishing\n", getpid());
            fflush(stdout);
            //Libero recursos 
            munmap(shm, sizeof(MemoriaCompartida));
            sem_destroy(&(shm->sem_empty));
            sem_destroy(&(shm->sem_fill));
            sem_destroy(&(shm->sem_mutex));
            exit(EXIT_SUCCESS);
        }
        //Subo semaforos 
        sem_post(&(shm->sem_mutex)); 
        sem_post(&(shm->sem_empty)); 

        i++; 
        // Espero <LAG> milisegundos
        usleep(lag * 1000); 
    }
}