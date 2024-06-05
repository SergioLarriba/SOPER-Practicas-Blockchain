/**
 * @file registrador.c
 * @authors Miguel Lozano y Sergio Larriba
 * @brief Implements a registrador that checks the result of mining operation.
 * @date 03-03-2023
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>

#include "pow.h"
#include "registrador.h"
#include "miner.h"

int registrador(int *pipe1, int id)
{
    ssize_t n_bytes = 0;
    long int buff;
    long target;
    char fileName[FILENAME_MAX];
    int fd;
    MemoriaCompartida *shm;

    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    // Especifico el tamaño de la memoria compartida
    // Mapeamos la memoria compartida a la estructura de memoria compartida
    shm = mmap(NULL, sizeof(MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    close(pipe1[1]);
    do
    {
        do
        {
            errno = 0;
            n_bytes = read(pipe1[0], &buff, sizeof(long));
        } while (n_bytes < 0 && errno == EINTR);
        if (n_bytes == 0)
        {
            break;
        }
        else if (n_bytes < 0)
        {
            perror("Monitor read error");
            close(pipe1[0]);
            close(fd);
            exit(EXIT_FAILURE);
        }
        sprintf(fileName, "%d", shm->minerInfo[id].pid);
        strcat(fileName, ".dat");
        fd = open(fileName, O_CREAT | O_WRONLY | O_APPEND, 0666);
        if (fd == -1)
        {
            perror("open");
            exit(EXIT_FAILURE);
        }
        if (shm->votes_yes >= shm->votes_no)
        {
            dprintf(fd, "Id: %d\n", shm->minerInfo[id].pid);
            dprintf(fd, "Winner: %d\n", shm->winner);
            dprintf(fd, "Target: %ld\n", shm->target);
            dprintf(fd, "Solution: %ld (acepted)\n", buff);
            dprintf(fd, "Votes: %d/%d\n", shm->votes_yes, (shm->votes_yes + shm->votes_no));
            dprintf(fd, "Coins in wallet: %ld\n\n", shm->minerInfo[id].n_coins);

            fprintf(stdout, "Solution accepted: %08ld --> %08ld\n", shm->target, buff);
            fflush(stdout);
        }
        else
        {
            dprintf(fd, "Id: %d\n", shm->minerInfo[id].pid);
            dprintf(fd, "Winner: %d\n", shm->winner);
            dprintf(fd, "Target: %ld\n", shm->target);
            dprintf(fd, "Solution: %ld (rejected)\n", buff);
            dprintf(fd, "Votes: %d/%d\n", shm->votes_yes, (shm->votes_yes + shm->votes_no));
            dprintf(fd, "Coins in wallet: %ld\n\n", shm->minerInfo[id].n_coins);

            fprintf(stdout, "Solution rejected: %08ld !-> %08ld\n", shm->target, buff);
            fflush(stdout);
        }
    } while (n_bytes != 0);
    close(pipe1[0]);
    close(fd);
    exit(EXIT_SUCCESS);
}