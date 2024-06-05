#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdbool.h>

#include "pow.h"
#include "miningSystem.h"

#define PRIME POW_LIMIT
#define BIG_X 435679812
#define BIG_Y 100001819

long int pow_hash(long int x) {
  long int result = (x * BIG_X + BIG_Y) % PRIME;
  return result;
}