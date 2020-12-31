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
#define DEBUG_STAMPA_MAPPA

/********** Variabili globali **********/
struct sembuf s_ops;
map *pointer_at_map;
int shm_id, sem_id;

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
	int i,j;
	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
			if (pointer_at_map->mappa[i][j].cell_type == 1){
				/* Sezione critica */
				s_ops.sem_num = 0;
				s_ops.sem_op = 
				s_ops.sem_flg = 0;
				semop(sem_id, &s_ops, 1);
				pointer_at_map->mappa[i][j].cell_type = 3;
				s_ops.sem_op = 
				semop(sem_id, &s_ops, 1);
			}
		}
	}
} 

/********** Main **********/
int main(int argc, char *argv[])
{
	/* Prendo l'indirizzo */ 
	shm_id = atoi(argv[1]);
	/* Mi attacco al segmento */
	pointer_at_map = shmat(shm_id, NULL, 0);
	/* Ottengo l'accesso al semaforo */
	sem_id = atoi(argv[2]);
	sem_id = semget(SEM_KEY, 1, 0600 | IPC_CREAT);
	/* Cerco una cella SO_SOURCE e mi attacco */
	attach(pointer_at_map);

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
