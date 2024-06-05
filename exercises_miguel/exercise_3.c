#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


int main(int argc, char const **argv) {

    FILE *pf = fopen(argv[1], "r");
    if (pf == NULL) {
        fprintf(stderr, "Error number: %d\n", errno);
        perror("Error description");
        return 1;
    }


    return 0;
}
