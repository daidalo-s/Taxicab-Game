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
#include <sys/msg.h>
#include "Map.h"

/********** VARIABILI GLOBALI **********/
/*  
 *	Deve accedere a: mappa in memoria condivisa, coda di messaggi della cella
 					 che sceglie, semaforo per active_taxis, semaforo per 
 					 crossings
 *	Deve ricordarsi di: dove si trova per scegliere il percorso.
 */	
int x, y;
int msg_queue_of_cell_key, map_shm_id;
map *pointer_at_map;


/********** ATTACH ALLA CELLA **********/
/*
 *	Con questo metodo il processo taxi si attacca a una cella in modo casuale che trova
 *	tra le libere e quelle che non eccedono già la capacità.
 *	Devo aggiungere due campi alla struct per SO_CAP_MIN e SO_CAP_MAX
 */
#if 0
void attach(map *pointer_at_map) {
    int x,y;
    srand(getpid());
    do {
        x = rand() % ((SO_HEIGHT-1) - 0 + 1) + 0; 
        y = rand() % ((SO_WIDTH-1) - 0 + 1) + 0;
    } while (pointer_at_map->mappa[x][y].cell_type == 0 && pointer_at_map->mappa[x][y].active_taxis < SO_CAP_MAX);
} 

#endif

/********** MAIN **********/
/*
 *	All'interno della funzione main il processo deve prendere controllo del segmento di memoria
 *	condivisa dove ho la mappa e dei vari semafori che userà per i suoi metodi.
 */
int main(int argc, char *argv[])
{
    /* Prendo l'id e mi attacco al segmento */ 
    map_shm_id = atoi(argv[1]);
    pointer_at_map = shmat(map_shm_id, NULL, 0);

    printf("Sono un processo Taxi \n");
    printf("Il campo della cella 2.2 e': %i \n", pointer_at_map->mappa[2][2].cell_type);


#ifdef DEBUG_STAMPA_MAPPA    
    printf("Uso il metodo di stampa tradizionale \n");
    map_print(pointer_at_map);
#endif

    printf("Ora perdo un po' di tempo e poi esco \n");
    sleep(3);
    printf("ho finito di dormire, sono un processo Taxi\n");
    return 0;
}


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
