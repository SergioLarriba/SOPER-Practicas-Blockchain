/**
 * @file voter.c
 * @author Miguel Lozano (miguel.lozanoa@estudiante.uam.es)
 * @author Sergio Larriba
 * @brief Voters of the voting system.
 * @date 2023-03-27
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h> 
#include <sys/types.h> 
#include <stdbool.h>
#include <errno.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>

#include "voter.h"
#include "names.h"
#define BUF_SIZE 64


bool start = false;
bool finish = false;

sem_t *semVote;
sem_t *semFinished;


/* Los procesos votantes pueden comenzar */
void sigusr1_handler() {
    start = true;
}

/* Comienza la votación */
void sigusr2_handler() {
    sem_t *semVoting;
    bool voto;
    int ret = 0;

    if (finish) {
        exit(EXIT_SUCCESS);
    }

    do {
        ret = sem_wait(semVote);
    } while (ret != 0);

    FILE *f = fopen(VOTE_COUNT_FILE, "a");
    if (f == NULL) {
        exit(EXIT_FAILURE);
    }

    srand(time(NULL) + getpid() * getpid());
    voto = rand() % 2;
    if (voto) {
        fprintf(f, "%d\n", 1);
    }
    else {
        fprintf(f, "%d\n", 0);
    }
    fclose(f);

    sem_post(semVote);
    sem_post(semFinished);
}

/* Finalizan los procesos votantes */
void sigterm_handler() {
    finish = true;
}

void voter(int n_procs, sem_t *semCandidate, sem_t *semVoting, sem_t *semFinish) {
    struct sigaction saUSR1;
    struct sigaction saUSR2;
    struct sigaction saTERM;
    sigset_t mask;
    int nVotes;
    int vote;

    FILE *fInfo = NULL;
    FILE *fVotes = NULL;
    pid_t pids[n_procs];
    char buf[BUF_SIZE];
    int vYes = 0;
    int ret = 0;

    semVote = semVoting;
    semFinished = semFinish;

    saUSR1.sa_handler = sigusr1_handler;
    sigemptyset(&saUSR1.sa_mask);
    saUSR1.sa_flags = 0;

    if (sigaction(SIGUSR1, &saUSR1, NULL) == -1) {
        perror("sigaction");
        sem_close(semCandidate);
        sem_close(semVoting);
        sem_close(semFinish);
        exit(EXIT_FAILURE);
    }

    saUSR2.sa_handler = sigusr2_handler;
    sigemptyset(&saUSR2.sa_mask);
    saUSR2.sa_flags = 0;

    if (sigaction(SIGUSR2, &saUSR2, NULL) == -1) {
        perror("sigaction");
        sem_close(semCandidate);
        sem_close(semVoting);
        sem_close(semFinish);
        exit(EXIT_FAILURE);
    }

    saTERM.sa_handler = sigterm_handler;
    sigemptyset(&saTERM.sa_mask);
    saTERM.sa_flags = 0;

    if (sigaction(SIGTERM, &saTERM, NULL) == -1) {
        perror("sigaction");
        sem_close(semCandidate);
        sem_close(semVoting);
        sem_close(semFinish);
        exit(EXIT_FAILURE);
    }

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    /* Wait for a signal to arrive. */
    sigprocmask(SIG_BLOCK, &mask, &saUSR1.sa_mask);
    while (!start) {
        sigsuspend(&saUSR1.sa_mask);
    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    if (start) {
        if ((fInfo = fopen(SYSTEM_INFO_FILE, "r")) == NULL) {
            perror("fopen");
            sem_close(semCandidate);
            sem_close(semVoting);
            sem_close(semFinish);
            exit(EXIT_FAILURE);
        }
        fgets(buf, BUF_SIZE, fInfo); /* Cadena leida y descartada: "N_PROCS = %d || N_SECS = %d\n" */
        fgets(buf, BUF_SIZE, fInfo); /* cadena leida y descartada: "PID de los procesos hijos:\n" */
        for (int i = 0; i < n_procs; i++) {
            fscanf(fInfo, "%d\n", &pids[i]);
        }
        fclose(fInfo);
    }
    while(start) {
        /* El que llega primero (Cadidato) entra, el resto (Votantes) esperan */
        do {
            ret = sem_wait(semCandidate);
            if (finish) {
                exit(EXIT_SUCCESS);
            }
        } while (ret != 0);

        if (finish) {
            break;
        }


        /* Creamos el fichero para registrar los votos o lo borramos si ya está creado */
        fVotes = fopen(VOTE_COUNT_FILE, "w");
        fclose(fVotes);

        /* Envía la señal para que comience la votacion*/
        for (int i = 0; i < n_procs; i++) {
            if (pids[i] != getpid()) {
                kill(pids[i], SIGUSR2);
            }
        }


        for (int i = 0; i < n_procs-1; i++)
        {
            sem_wait(semFinish);
        }
        do {
            sleep(0.001);
            sem_getvalue(semFinish, &nVotes);
        } while (nVotes != n_procs);
        
        /* Una vez la votación ha finalizado, se cuentan los votos */
        vYes = 0;
        printf("Candidate %d => [ ", (int)getpid());
        fflush(stdout);
        fVotes = fopen(VOTE_COUNT_FILE, "r");
        for (int i = 0; i < n_procs-1; i++) {
            fscanf(fVotes, "%d\n", &vote);
            if (vote) {
                printf("Y ");
                vYes++;
            } else {
                printf("N ");
            }
            fflush(stdout);
        }
        fclose(fVotes);

        printf("] => ");
        if (vYes > n_procs-1-vYes) {
            printf("Accepted\n");
        } else {
            printf("Rejected\n");
        }
        fflush(stdout);
        
        sleep(0.25);
        sem_post(semCandidate);
        
        if (finish) {
            break;
        }
    }

    sem_close(semCandidate);
    sem_close(semVoting);
    sem_close(semFinish);

    exit(EXIT_SUCCESS);
} 