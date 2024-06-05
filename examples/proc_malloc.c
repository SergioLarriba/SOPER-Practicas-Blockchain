#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MESSAGE "Hello"

int main(void) {
  pid_t pid;
  char *sentence = calloc(sizeof(MESSAGE), 1);

  pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    strcpy(sentence, MESSAGE);
    free(sentence); 
    exit(EXIT_SUCCESS);
  } else {
    wait(NULL);
    printf("Parent: %s\n", sentence);
    free(sentence); 
    exit(EXIT_SUCCESS);
  }
}
