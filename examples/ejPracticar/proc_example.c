#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_PROC 3

int main(void) {
  int i;
  pid_t pid;

  for (i = 0; i < NUM_PROC; i++) {
    pid = fork();
    exit(EXIT_SUCCESS); // Pongo esto para que P1 solo cree un proceso, q es P2. 
    if (pid < 0) {
      perror("fork");
      exit(EXIT_FAILURE);
    } else if (pid == 0) { // Ha creado P2 un hijo, q es P3
      printf("Child -> pid: %d \n", getpid());
    } else if (pid > 0) {
      printf("Parent -> pid padre: %d\n", getppid());
      wait(NULL); // Espero al hijo, y asi no dejo huerfanos 
    }
  }
  wait(NULL);
  exit(EXIT_SUCCESS);
}

/* getpid() -> pid del proceso 
   getppid() -> pid del proceso padre 
   el padre sabe el pid del hijo en cuanto llama a fork, es lo q le devuelve 
   si tiene muchos hijos el padre, sabe todos los pids de sus hijos con un array de pids */