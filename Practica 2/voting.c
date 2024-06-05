/**
 * @file voting.c
 * @author Miguel Lozano (miguel.lozanoa@estudiante.uam.es)
 * @author Sergio Larriba
 * @brief Main process of voting system.
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
#include <fcntl.h>

#include "voter.h"
#include "names.h"


bool usr_interrupt = false;
int *pids = NULL;
bool alarm_interrupt = false;

/**
 * @brief Function handler for SIGINT signal.
 * 
 */
void handlerSIGINT() {
    usr_interrupt = true;
}

/**
 * @brief Function handler for SIGALRM signal.
 * 
 */
void handlerSIGALRM() {
    alarm_interrupt = true;
}

int main (int argc, char const **argv)
{
    int n_procs=0, n_secs=0, i, pid; 
    FILE *f=NULL; 
    struct sigaction saSIGINT;
    struct sigaction saSIGALRM;
    sigset_t mask;
    sem_t *semCandidate;
    sem_t *semVoting;
    sem_t *semVotationFinished;

    if (argc != 3) {
        fprintf(stderr, "ERROR: Invalid number of arguments\nUse this format:\n./voting <N_PROCS> <N_SECS>\n");
        return EXIT_FAILURE;
    }
    
    n_procs = atoi(argv[1]); 
    n_secs = atoi(argv[2]); 
    
    saSIGINT.sa_handler = handlerSIGINT;
    sigemptyset(&(saSIGINT.sa_mask));
    saSIGINT.sa_flags = 0;

    saSIGALRM.sa_handler = handlerSIGALRM;
    sigemptyset(&(saSIGALRM.sa_mask));
    saSIGALRM.sa_flags = 0;

    if (sigaction(SIGINT, &saSIGINT, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }


    if (sigaction(SIGALRM, &saSIGALRM, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Abro el fichero donde guardare los datos */
    if ((f = fopen(SYSTEM_INFO_FILE, "w")) == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* creo un array de pid */
    pids = (int*) malloc (sizeof(int) * n_procs); 
    if (pids == NULL) {
        exit(EXIT_FAILURE); 
    }

    /* Abro el fichero donde guardare los datos */
    if ((f = fopen(SYSTEM_INFO_FILE, "w")) == NULL) {
        free(pids);
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    sem_unlink(CANDIDATE_SEM_NAME);
    sem_unlink(VOTING_SEM_NAME);
    sem_unlink(VOTATION_FINISHED_SEM_NAME);
    semCandidate = sem_open(CANDIDATE_SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
    semVoting = sem_open(VOTING_SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
    semVotationFinished = sem_open(VOTATION_FINISHED_SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, n_procs);

    /* Creo n_procs Votante y almaceno la informacion en un fichero */
    fprintf(f, "N_PROCS = %d || N_SECS = %d\n", n_procs, n_secs); 
    fprintf(f, "PID de los procesos hijos:\n");
    fflush(f);
    for (i=0; i<n_procs; i++) {
        pid = fork();
        pids[i] = pid;
        switch (pid) {
            case -1:
                perror("fork error");
                fclose(f);
                free(pids);
                sem_close(semCandidate);
                sem_close(semVoting);
                sem_close(semVotationFinished);
                sem_unlink(CANDIDATE_SEM_NAME);
                sem_unlink(VOTING_SEM_NAME);
                sem_unlink(VOTATION_FINISHED_SEM_NAME);
                exit(EXIT_FAILURE);
                break;
            
            case 0:
                voter(n_procs, semCandidate, semVoting, semVotationFinished);
                exit(EXIT_SUCCESS);

            default:
                fprintf(f, "%d\n", pids[i]);
                fflush(f);
                break;
            }
    }
    fclose(f);
    if (usr_interrupt == true) {
        for (int i = 0; i<n_procs; i++) {
            kill(pids[i], SIGTERM);
        }
        for (int i = 0; i<n_procs; i++) {
            wait(NULL);
        }
        free(pids);
        printf("Finishing by signal\n");
    }


    /* El proceso principal enviara la señal SIGUSR1 a todos los procesos voter */
    for (i=0; i<n_procs; i++) {
        kill(pids[i], SIGUSR1); 
    }

    alarm(n_secs);

    /* El proceso principal espera a recibir la señal SIGINT (sigsuspend)*/
    /* Set up the mask of signals to temporarily block. */
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGALRM);
    /* Wait for a signal to arrive. */
    sigprocmask(SIG_BLOCK, &mask, &saSIGINT.sa_mask);
    while (!usr_interrupt && !alarm_interrupt) {
        sigsuspend(&saSIGINT.sa_mask);
    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);


    for (int i = 0; i<n_procs; i++) {
        kill(pids[i], SIGTERM);
    }

    for (int i = 0; i<n_procs; i++) {
        wait(NULL);
    }

    free(pids);

    sem_close(semCandidate);
    sem_close(semVoting);
    sem_close(semVotationFinished);
    sem_unlink(CANDIDATE_SEM_NAME);
    sem_unlink(VOTING_SEM_NAME);
    sem_unlink(VOTATION_FINISHED_SEM_NAME);

    if (alarm_interrupt == true) {
        printf("Finishing by alarm\n");
    } else if (usr_interrupt == true) {
        printf("\nFinishing by signal\n");
    }
    return EXIT_SUCCESS;
}
