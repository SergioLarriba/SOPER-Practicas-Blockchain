/**
 * @file monitor.c
 * @authors Miguel Lozano y Sergio Larriba
 * @brief Implements a monitor that checks the result of mining operation.
 * @date 03-03-2023
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>

#include "miningSystem.h"
#include "pow.h"
#include "monitor.h"

int monitor(int *pipe1, int *pipe2, long int target) {
    ssize_t n_bytes = 0;
    long int buff;

    close(pipe1[1]);
    close(pipe2[0]);
    do {
    n_bytes = read(pipe1[0], &buff, sizeof(long));
    if (n_bytes == 0) {
        break;
    }
    if (pow_hash(buff) == target) {
        fprintf(stdout, "Solution accepted: %08ld --> %08ld\n", target, buff);
        fflush(stdout);
        target = buff;
        write(pipe2[1], "OK\n", 3);
    }
    else {
        fprintf(stdout, "Solution rejected: %08ld !-> %08ld\n", target, buff);
        fflush(stdout);
        write(pipe2[1], "ERROR\n", 6);
    }
    if(n_bytes < 0) {
        perror("Monitor read error");
        close(pipe1[0]);
        close(pipe2[1]);
        exit(EXIT_SUCCESS);
    }
    } while (n_bytes != 0);
    close(pipe1[0]);
    close(pipe2[1]);
    exit(EXIT_SUCCESS);
}