/**
 * @file miner.c
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
#include <stdbool.h>
#include <mqueue.h>
#include <sys/stat.h>

#include "pow.h"
#include "registrador.h"
#include "miner.h"

#define MQ_NAME "/MyMessageQueue"

/* Global variables */
int work_per_thread, n_threads_global;
long int target, next_target, thread_result;
Message message;
MemoriaCompartida *memoriaCompartida; 

/* Private functions */

/**
 * @brief Iterate the pow_hash function for different inputs.
 *
 * @param arg The id associated to a thread.
 */
void *mining(void *arg);

/* Public functions */
int main(int argc, char **argv /*int seconds, int n_threads*/)
{
  pthread_t *h = NULL;
  int i, j, error, nbytes = 0;
  int *ids = NULL;
  void *result = NULL;
  int pipe1[2];
  pid_t pid;
  char buff_char[BUFF_SIZE];
  int status = EXIT_SUCCESS;
  pid_t winner;
  mqd_t mqdes;
  int fd; 
  MemoriaCompartida *shm; //Puntero a la memoria compartida

  if (argc != 3)
  {
    fprintf(stderr, "Error in miner args\n");
    return EXIT_FAILURE;
  }

  n_threads_global = atoi(argv[2]);

  ids = (int *)malloc(sizeof(int) * n_threads_global);
  if (ids == NULL)
  {
    return EXIT_FAILURE;
  }
  for (i = 0; i < n_threads_global; i++)
  {
    ids[i] = i;
  }

  h = (pthread_t *)malloc(n_threads_global * sizeof(h[0]));
  if (h == NULL)
  {
    fprintf(stderr, "ERROR when creating the array of threads\n");
    free(ids);
    return EXIT_FAILURE;
  }
  //Estructura de la cola de mensajes 
  struct mq_attr mq_struct = {
      .mq_flags = 0,
      .mq_maxmsg = MAX_NUM_MESSAGES,
      .mq_msgsize = sizeof(Message),
      .mq_curmsgs = 0,
  };
  //Abrimos la cola de mensajes 
  mqdes = mq_open(MQ_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &mq_struct);
  if (mqdes == -1)
  {
    perror("mq_open");
    free(ids);
    exit(EXIT_FAILURE);
  }

  work_per_thread = POW_LIMIT / n_threads_global;

  target = 0;

  if (pipe(pipe1) == -1)
  {
    perror("pipe error");
    free(h);
    free(ids);
    exit(EXIT_FAILURE);
  }
  //Abro el segmento de memoria compartida con la informacion del sistema, detectando si es el primer minero o no 
  if ((fd = shm_open(SHM_NAME, O_CREAT  | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR)) == -1) {
    /* No es el primer minero */
    if (errno == EEXIST) { 
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
    } 
    else {
      perror("shm_open"); 
      exit(EXIT_FAILURE); 
    }
  } else { /* Es el primer minero */
    //Abrimos la memoria compartida y lo enlazamos a la estructura de memoria compartida
    fd = shm_open(SHM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
      perror("shm_open");
      free(h);
      free(ids);
      exit(EXIT_FAILURE);
    }
    //Especifico el tamaño de la memoria compartida 
    //Asigno una region de memoria virtual 
    shm = mmap(NULL, sizeof(MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) {
      perror("mmap"); 
      exit(EXIT_FAILURE); 
    }
    //Mapeamos la memoria compartida a la estructura de memoria compartida
    shm = mmap(NULL, sizeof(MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (h == MAP_FAILED) {
      perror("mmap");
      free(h);
      free(ids);
      exit(EXIT_FAILURE);
    } 
  }

  pid = fork();

  switch (pid)
  {
  case -1:
    perror("fork error");
    free(h);
    free(ids);
    exit(EXIT_FAILURE);
    break;
  case 0: /* REGISTRADOR */
    free(ids);
    free(h);
    registrador(pipe1, target, getpid());
    break;
  default: /* MINER */
    close(pipe1[0]);
    while (true)
    {
      for (i = 0; i < n_threads_global; i++)
      {
        error = pthread_create(&h[i], NULL, mining, &ids[i]);
        if (error != 0)
        {
          fprintf(stderr, "pthread_create: ERROR\n");
          free(ids);
          free(h);
          return EXIT_FAILURE;
        }
      }

      for (i = 0; i < n_threads_global; i++)
      {
        error = pthread_join(h[i], &result);
        if (result != NULL)
        {
          winner = getpid();
          /* Enviar a Registrador */
          write(pipe1[1], (long int *)result, sizeof(long int)); /* Escribir resultado */
          write(pipe1[1], &winner, sizeof(pid_t));
          /* Enviar a Comprobador */
          message.objetivo = target;
          message.solucion = (long)result;
          message.valido = 0;
          nbytes = mq_send(mqdes, (char *)&message, sizeof(Message), 0);
          if (nbytes == -1)
          {
            close(pipe1[1]);
            mq_close(mqdes);
            mq_unlink(MQ_NAME);
            free(ids);
            free(h);
            perror("mq_send");
            exit(EXIT_FAILURE);
          }
        }
        if (error != 0)
        {
          fprintf(stderr, "pthread_join: ERROR\n");
          close(pipe1[1]);
          wait(NULL);
          mq_close(mqdes);
          mq_unlink(MQ_NAME);
          free(ids);
          free(h);
          return (EXIT_FAILURE);
        }
      }
      target = next_target;
    }
    close(pipe1[1]);
    break;
  }
  wait(&status);
  if (status)
  {
    fprintf(stderr, "Registrador exited unexpectedly\n");
    fflush(stderr);
  }
  else
  {
    printf("Registrador exited with status %d\n", status);
  }
  mq_close(mqdes);
  mq_unlink(MQ_NAME);
  free(ids);
  free(h);
  return EXIT_SUCCESS;
}

void *mining(void *arg)
{
  int i;
  int id = *((int *)arg);
  int end = (id + 1) * work_per_thread;

  if (id == (n_threads_global - 1))
  {
    end = POW_LIMIT + 1;
  }

  for (i = id * work_per_thread; i < end; i++)
  {
    if (pow_hash(i) == target)
    {
      next_target = i;
      thread_result = i;
      pthread_exit(&thread_result);
    }
  }

  pthread_exit(NULL);
}