/**
 * @file miner.c
 * @authors Miguel Lozano y Sergio Larriba
 * @brief Implements a mining system to run a proof of work.
 * @date 01-03-2023
 */

#define _GNU_SOURCE

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
#include "monitor.h"
#include "miner.h"

#define MQ_NAME "/MyMessageQueue"

/* Global variables */
int work_per_thread, n_threads_global;
long int target, next_target, thread_result;
Message message;
MemoriaCompartida *shm;
int id;
bool start = false;
bool loser = false;
int yes = 0, no = 0;

/* Private functions */

/**
 * @brief Iterate the pow_hash function for different inputs.
 *
 * @param arg The id associated to a thread.
 */
void *mining(void *arg);

int sem_wait_protected(sem_t *semaphore)
{
  while (sem_wait(semaphore) < 0)
  {
    if (errno != EINTR)
    {
      return -1;
    }
  }

  return 0;
}

/* Handlers */

/* El minado puede comenzar */
void sigusr1_handler()
{
  start = true;
}

/* ¡Votación! */
void sigusr2_handler()
{
  loser = true;
}

/* FIN */
void sigalrm_handler()
{
  exit(EXIT_SUCCESS);
}

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
  int fd; // Puntero a la memoria compartida
  struct sigaction saUSR1;
  struct sigaction saUSR2;
  struct sigaction saALRM;
  sigset_t mask;
  MemoriaCompartida *shm;
  int n_secs;

  if (argc != 3)
  {
    fprintf(stderr, "Error in miner args\n");
    return EXIT_FAILURE;
  }

  n_secs = atoi(argv[1]);
  n_threads_global = atoi(argv[2]);

  /* Control de la señal SIGUSR1 */
  saUSR1.sa_handler = sigusr1_handler;
  sigemptyset(&saUSR1.sa_mask);
  saUSR1.sa_flags = 0;
  if (sigaction(SIGUSR1, &saUSR1, NULL) == -1)
  {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  /* Control de la señal SIGUSR1 */
  saUSR2.sa_handler = sigusr2_handler;
  sigemptyset(&saUSR2.sa_mask);
  saUSR2.sa_flags = 0;
  if (sigaction(SIGUSR2, &saUSR2, NULL) == -1)
  {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  saALRM.sa_handler = sigalrm_handler;
  sigemptyset(&saALRM.sa_mask);
  saALRM.sa_flags = 0;
  if (sigaction(SIGALRM, &saALRM, NULL) == -1)
  {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

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
  // Estructura de la cola de mensajes
  struct mq_attr mq_struct = {
      .mq_flags = 0,
      .mq_maxmsg = MAX_NUM_MESSAGES,
      .mq_msgsize = sizeof(Message),
      .mq_curmsgs = 0,
  };
  // Abrimos la cola de mensajes
  mqdes = mq_open(MQ_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &mq_struct);
  if (mqdes == -1)
  {
    perror("mq_open");
    free(ids);
    exit(EXIT_FAILURE);
  }

  work_per_thread = POW_LIMIT / n_threads_global;

  if (pipe(pipe1) == -1)
  {
    perror("pipe error");
    free(h);
    free(ids);
    exit(EXIT_FAILURE);
  }
  // Abro el segmento de memoria compartida con la informacion del sistema, detectando si es el primer minero o no
  if ((fd = shm_open(SHM_NAME, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR)) == -1)
  {
    /* No es el primer minero */
    if (errno == EEXIST)
    {
      // Abro la memoriaCompartidaCompartida compartida en modo lectura
      fd = shm_open(SHM_NAME, O_RDWR, 0);
      if (fd == -1)
      {
        perror("shm_open");
        exit(EXIT_FAILURE);
      }
      // Extraigo el bloque (Leo)
      shm = mmap(NULL, sizeof(MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      if (shm == MAP_FAILED)
      {
        perror("mmap");
        exit(EXIT_FAILURE);
      }
    }
    else
    {
      perror("shm_open");
      exit(EXIT_FAILURE);
    }
  }
  else
  { /* Es el primer minero */
    // Abrimos la memoria compartida y lo enlazamos a la estructura de memoria compartida
    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
      perror("shm_open");
      free(h);
      free(ids);
      exit(EXIT_FAILURE);
    }
    // Especifico el tamaño de la memoria compartida
    ftruncate(fd, sizeof(MemoriaCompartida));
    // Mapeamos la memoria compartida a la estructura de memoria compartida
    shm = mmap(NULL, sizeof(MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (h == MAP_FAILED)
    {
      perror("mmap");
      free(h);
      free(ids);
      exit(EXIT_FAILURE);
    }
    if (sem_init(&(shm->sem_minerInfo), 1, 1) == -1)
    {
      perror("sem_init");
      free(ids);
      free(h);
      exit(EXIT_FAILURE);
    }
    sem_wait_protected(&(shm->sem_minerInfo));
    shm->n_miners = 0;
    shm->target = 0;
    sem_post(&(shm->sem_minerInfo));
  }

  /* El minero se registra en el sistema */
  sem_wait_protected(&(shm->sem_minerInfo));
  if (shm->n_miners >= MAX_MINERS)
  {
    sem_post(&(shm->sem_minerInfo));
    free(ids);
    free(h);
    exit(EXIT_FAILURE);
  }
  id = shm->n_miners;
  shm->minerInfo[shm->n_miners].n_coins = 0;
  shm->minerInfo[shm->n_miners].pid = getpid();
  shm->n_miners++;
  sem_post(&(shm->sem_minerInfo));

  alarm(n_secs);
  /* Creo registrador */
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
    registrador(pipe1, id);
    break;
  default: /* MINER */
    while (true)
    {
      sem_wait_protected(&(shm->sem_minerInfo));
      if (!start)
      {
        start = true;
        for (int i = 0; i < shm->n_miners; i++)
        {
          if (!(shm->minerInfo[i].pid == getpid()))
          {
            kill(shm->minerInfo[i].pid, SIGUSR1);
          }
        }
      }
      sem_post(&(shm->sem_minerInfo));
      /* Esperamos a SIGUSR1 para empezar a minar */
      if (!start)
      {
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        sigprocmask(SIG_BLOCK, &mask, &saUSR1.sa_mask);
        while (!start)
        {
          sigsuspend(&saUSR1.sa_mask);
        }
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
      }
      /* Empezamos a minar */
      close(pipe1[0]);

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
        if (result != NULL && loser == false)
        {
          sem_wait_protected(&(shm->sem_minerInfo));
          start = false;
          shm->solution = (long)result;
          for (int i = 0; i < shm->n_miners; i++)
          {
            if (shm->minerInfo[i].pid != getpid())
            {
              kill(shm->minerInfo[i].pid, SIGUSR2);
            }
          }
          sem_post(&(shm->sem_minerInfo));
          /* Contar votaciones */
          for (int i = 0; i < N_TIMES; i++)
          {
            for (int i = 0; i < shm->n_miners; i++)
            {

              if (shm->minerInfo[i].vote == true)
              {
                shm->votes_yes++;
              }
              else
              {
                shm->votes_no++;
              }
              shm->winner = getpid();
              sleep(0.1);
            }
            write(pipe1[1], (long int *)result, sizeof(long int)); /* Escribir resultado */
            shm->minerInfo[id].n_coins++;
            shm->target = next_target;
            shm->votes_no = 0;
            shm->votes_yes = 0;
          }
        }
        else if (loser == true) /* Proceso perdedor */
        {
          start = false;
          loser = false;
          if (shm->target == pow_hash(shm->solution))
          {
            shm->minerInfo[id].vote = true;
          }
          else
          {
            shm->minerInfo[id].vote = false;
          }
          break;
        }

        /* TODO: EL ENVIO AL REGISTRADOR LO DEBEN HACER TODOS LOS MINEROS TRAS FINALIZAR LA VOTACION */
        /* Enviar a Registrador */

        /* Enviar a Monitor */
        // message.objetivo = target;
        // message.solucion = (long)result;
        // message.valido = 0;
        // nbytes = mq_send(mqdes, (char *)&message, sizeof(Message), 0);
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
    /* TODO: TRAS LA VOTACION EL WINNER DEBE ASIGNAR EL TARGET */

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
    if (loser == true)
    {
      break;
    }
    if (pow_hash(i) == target)
    {
      next_target = i;
      thread_result = i;
      pthread_exit(&thread_result);
    }
  }

  pthread_exit(NULL);
}