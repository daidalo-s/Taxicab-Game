/*Stiamo importando tutte le librerie necessarie?*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include <errno.h> 
#include <time.h>  
#include <unistd.h> 

int main(int argc, char *argv[])
{
    /* code */
    printf("Sono un processo SO_SOURCE \n");
    printf("Questa e' una stampa di test di SO_SOURCE \n");
    printf("Ora perdo un po' di tempo e poi esco \n");
    sleep(10);
    printf("Ho finito di dormire \n");
    return 0;
}
