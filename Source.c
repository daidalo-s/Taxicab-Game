/*Stiamo importando tutte le librerie necessarie?*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include <errno.h> 
#include <time.h>  
#include <unistd.h> 
#include "Map.h"
#include <sys/shm.h>

void map_print(map *pointer_at_map) {
	int i, j;
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			printf ("%i ", pointer_at_map->mappa[i][j].cell_type);
		}
		printf("\n");
	}
}

int main(int argc, char *argv[])
{
	map *pointer_at_map;
	int shmid;
	/* Prendo l'indirizzo */ 
	shmid = atoi(argv[1]);
	/* Mi attacco al segmento */
	pointer_at_map = shmat(shmid, NULL, 0);

    printf("Sono un processo SO_SOURCE \n");
    printf("Il campo della cella 2.2 e': %i \n", pointer_at_map->mappa[2][2].cell_type);

#ifdef DEBUG_STAMPA_MAPPA    
    printf("Uso il metodo di stampa tradizionale \n");
    map_print(pointer_at_map);
#endif

    printf("Ora perdo un po' di tempo e poi esco \n");
    sleep(2);
    printf("Ho finito di dormire \n");
    return 0;
}
