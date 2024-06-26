#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

/* Handler function for the signal SIGINT. */
void handler(int sig) {
  printf("Signal number %d received\n", sig);
  fflush(stdout);
}

int main(void) {
  struct sigaction act;

  act.sa_handler = handler;
  sigemptyset(&(act.sa_mask));
  act.sa_flags = 0;

  if (sigaction(SIG_IGN, &act, NULL) < 0) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  while (1) {
    printf("Waiting Ctrl+C (PID = %d)\n", getpid());
    sleep(9999);
  }
}
