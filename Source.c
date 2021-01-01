/*Stiamo importando tutte le librerie necessarie?*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/types.h> 
#include <sys/wait.h>
#include <errno.h> 
#include <time.h>  
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/sem.h>
#include "Map.h"

/********** Variabili globali **********/
map *pointer_at_map;
int shm_id, sem_id;
struct sembuf accesso = { 0, -1, 0}; /* semwait */
struct sembuf rilascio = { 0, +1, 0}; /* semsignal */

/********** Metodi per debug **********/
void map_print(map *pointer_at_map) {
    int i, j;
    for (i = 0; i < SO_HEIGHT; i++) {
        for (j = 0; j < SO_WIDTH; j++) {
            printf ("%i ", pointer_at_map->mappa[i][j].cell_type);
        }
        printf("\n");
    }
}

/********** Attach alla cella **********/
void attach(map *pointer_at_map) {
    int i,j,debug;
    for (i = 0; i < SO_HEIGHT; i++){
        for (j = 0; j < SO_WIDTH; j++){
            if (pointer_at_map->mappa[i][j].cell_type == 1){
                /* Sezione critica */
                printf("Fino a prima della semop arrivo \n"); 
                debug = semctl(sem_id, 0, GETVAL);
                printf("%i \n", debug);
                semop(sem_id, &accesso, 1);
                printf("Dopo il blocco della risorsa eseguo \n");
                pointer_at_map->mappa[i][j].cell_type = 3;
                debug = semctl(sem_id, 0, GETVAL);
                printf("Durante il blocco vale %i \n", debug);
                /* Rilascio la risorsa */
                semop(sem_id, &rilascio, 1);
                debug = semctl(sem_id, 0, GETVAL);
                printf("Dopo il rilascio vale %i \n", debug);

            }
        }
    }
} 

/********** Main **********/
int main(int argc, char *argv[])
{	
    sleep(5);
    /* Prendo l'indirizzo */ 
    shm_id = atoi(argv[1]);
    /* Mi attacco al segmento */
    pointer_at_map = shmat(shm_id, NULL, 0);
    /* Ottengo l'accesso al semaforo */
    sem_id = semget(SEM_KEY, 1, 0600);
    printf("L'id del semaforo che ho in source è %i \n", sem_id);
    /* Cerco una cella SO_SOURCE e mi attacco */
#if 1
    attach(pointer_at_map);
#endif
    printf("Sono un processo SO_SOURCE \n");
    printf("Il campo della cella 2.2 e': %i \n", pointer_at_map->mappa[2][2].cell_type);

#ifdef DEBUG_STAMPA_MAPPA  
    printf("Uso il metodo di stampa tradizionale \n");
    map_print(pointer_at_map);
#endif

    printf("Ora perdo un po' di tempo e poi esco \n");
    sleep(2);
    printf("Ho finito di dormire sono un processo Source \n");
    return 0;
}
