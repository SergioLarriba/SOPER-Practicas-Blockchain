#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main (int argc, char **argv)
{
    FILE *f=NULL; 
    int mi_error; 

    f = fopen ("r", argv[0]); 
    mi_error = errno; 
    printf ("%d\n", mi_error); 
    if (f == NULL)
    {
        printf ("Error: %d", errno); 
        perror("El errno significa: "); 
        return 1; 
    }
    return 0; 
}



