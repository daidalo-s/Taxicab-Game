#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include <errno.h> 
#include <time.h>  
#include <unistd.h> 

int main(int argc, char *argv[])
{
    /* code */
    printf("Sono un processo Taxi \n");
    printf("Questa e' una stampa di test di SO_TAXI\n");
    printf("Ora perdo un po' di tempo e poi esco \n");
    sleep(15);
    printf("ho finito di dormire, sono un processo Taxi\n");
    return 0;
}
