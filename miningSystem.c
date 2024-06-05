/**
 * @file miningSystem.c
 * @authors Miguel Lozano y Sergio Larriba
 * @brief Implements a mining system to run a proof of work.
 * @date 01-03-2023
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

#define BUFF_SIZE 64

/* Global variables */
int work_per_thread, n_threads_global;
long int target, next_target, thread_result;

/* Private functions */

/**
 * @brief Iterate the pow_hash function for different inputs.
 * 
 * @param arg The id associated to a thread.
*/
void *miner(void *arg);


/* Public functions */
int miningSystem(int target_ini, int rounds, int n_threads) {
  pthread_t *h = NULL;
  int i, j, error;
  int *ids = NULL;
  void *result = NULL;
  int pipe1[2], pipe2[2];
  pid_t pid;
  char buff_char[BUFF_SIZE];
  int status = EXIT_SUCCESS;
  
  if ((target_ini < 0) || (target_ini >= POW_LIMIT) || (rounds < 0) || (n_threads <= 0))
  {
    fprintf(stderr, "Error in miningSystem args\n");
    return EXIT_FAILURE;
  }
  
  n_threads_global = n_threads;

  ids = (int*) malloc(sizeof(int) * n_threads);
  if (ids == NULL) {
    return EXIT_FAILURE;
  }
  for (i=0; i<n_threads; i++) {
    ids[i] = i;
  }

  h = (pthread_t*) malloc(n_threads * sizeof(h[0])); 
  if (h == NULL) {
    fprintf(stderr, "ERROR when creating the array of threads\n"); 
    free(ids);
    return EXIT_FAILURE;
  }

  work_per_thread = POW_LIMIT / n_threads;

  target = target_ini;

  if(pipe(pipe1) == -1) {
    perror("pipe error");
    free(h);
    free(ids);
    exit(EXIT_FAILURE);
  }
  if(pipe(pipe2) == -1) {
    perror("pipe error");
    free(h);
    free(ids);
    exit(EXIT_FAILURE);
  }
  pid = fork();
  
  switch (pid) {
    case -1:
      perror("fork error");
      free(h);
      free(ids);
      exit(EXIT_FAILURE);
      break;
    case 0: /* MONITOR */
      free(ids);
      free(h);
      monitor(pipe1, pipe2, target);
      break;
    default: /* MINER */
      close(pipe1[0]);
      close(pipe2[1]);
      for (j=0; j<rounds; j++) {
        for (i=0; i<n_threads; i++) {
          error = pthread_create(&h[i], NULL, miner, &ids[i]); 
          if (error != 0) {
            fprintf(stderr, "pthread_create: ERROR\n");
            free(ids);
            free(h);
            return EXIT_FAILURE;
          }   
        } 
        
        for (i=0; i<n_threads; i++) {
          error = pthread_join(h[i], &result);
          if (result != NULL) {
            /* TODO: Enviar resultado a Monitor */
            write(pipe1[1], (long int*)result, sizeof(long int));
            read(pipe2[0], &buff_char, BUFF_SIZE);
            if (strncmp(buff_char, "ERROR", 5) == 0) {
              fprintf(stderr, "The solution has been invalidated\n");
              fflush(stderr);
              close(pipe1[1]);
              close(pipe2[0]);
              wait(NULL);
              free(ids);
              free(h);
              return(EXIT_FAILURE);
            }
          }
          if (error != 0) {
            fprintf(stderr, "pthread_join: ERROR\n");
            close(pipe1[1]);
            close(pipe2[0]);
            wait(NULL);
            free(ids);
            free(h);
            return(EXIT_FAILURE);
          }
        }
        target = next_target;
      }
      close(pipe1[1]);
      close(pipe2[0]);
      break;
    
    
  }
  wait(&status);
  if(status) {
    fprintf(stderr, "Monitor exited unexpectedly\n");
    fflush(stderr);
  }
  else {
    printf("Monitor exited with status %d\n", status);
  }
  free(ids);
  free(h);
  return EXIT_SUCCESS;
}

void *miner(void *arg) { 
  int i;
  int id = *((int*)arg);
  int end = (id+1) * work_per_thread;
  
  if (id == (n_threads_global-1)) {
    end = POW_LIMIT+1;
  }

  for (i=id * work_per_thread; i < end; i++) {
    if (pow_hash(i)==target) { 
      next_target = i;
      thread_result = i;
      pthread_exit(&thread_result); 
    }
  }

  pthread_exit(NULL);
}