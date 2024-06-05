/**
 * @file mrush.c
 * @authors Miguel Lozano y Sergio Larriba
 * @brief
 * @date 07-02-2023
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

#include "pow.h"
#include "miningSystem.h"

int main(int argc, char const **argv) {
  int target_ini, rounds, n_threads, ret_cod_pid;
  pid_t pid;
  int status = EXIT_SUCCESS;
  
  if (argc != 4) {
    fprintf(stderr, "ERROR: Invalid number of arguments\nUse this format:\n./mrush <TARGET_INI> <ROUNDS> <N_THREADS>\n");
    return EXIT_FAILURE;
  }

  target_ini = atoi(argv[1]);
  rounds = atoi(argv[2]);
  n_threads = atoi(argv[3]); 

  pid = fork();
  switch (pid)
  {
  case -1:
    perror("fork error");
    fflush(stdout);
    exit(EXIT_FAILURE);
    break;
  
  case 0:
    status = miningSystem(target_ini, rounds, n_threads);
    if(status) {
      exit(EXIT_FAILURE);
    }
    break;
  
  default:
    wait(&ret_cod_pid);
    if(ret_cod_pid) {
      fprintf(stderr, "Miner exited unexpectedly\n");
      fflush(stderr);
    }
    else {
      printf("Miner exited with status %d\n", ret_cod_pid);
    }
    break;
  }

  exit(EXIT_SUCCESS);
}