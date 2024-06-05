#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  //char *argv[3] = {"mi-ls", "./", NULL};
  char *list0 = "mi-ls"; 
  char *list1 = "./";
  char *list2 = NULL;
  pid_t pid;

  pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    if (execl("/usr/bin/ls", list0, list1, list2)) {
      perror("execvp");
      exit(EXIT_FAILURE);
    }
  } else {
    wait(NULL);
  }
  exit(EXIT_SUCCESS);
}