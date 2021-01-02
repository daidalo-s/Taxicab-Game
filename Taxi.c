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
 *      che sceglie, semaforo per active_taxis, semaforo per crossings
 *	Deve ricordarsi di: dove si trova per scegliere il percorso.
 */	
int x, y;
int random_coordinates[2];
int tmpx, tmpy;
int msg_queue_of_cell_key, msg_queue_of_cell, map_shm_id, taxi_sem_id;
struct sembuf accesso;
struct sembuf rilascio;    
map *pointer_at_map;


void random_cell() {
	srand(getpid());
    /* Possibile loop infinito, dipende dai controlli */
    do {
        random_coordinates[0] = rand() % ((SO_HEIGHT-1) - 0 + 1) + 0; /* x */
        random_coordinates[1] = rand() % ((SO_WIDTH-1) - 0 + 1) + 0; /* y */
    } while (pointer_at_map->mappa[random_coordinates[0]][random_coordinates[1]].cell_type == 0);
    tmpx = random_coordinates[0];
    tmpy = random_coordinates[1];
    /* Impostazione di accesso */
    accesso.sem_num = pointer_at_map->mappa[tmpx][tmpy].reference_sem_number; 
	accesso.sem_op = -1;
	accesso.sem_flg = IPC_NOWAIT;
	/* Impostazione di rilascio */
	rilascio.sem_num = pointer_at_map->mappa[tmpx][tmpy].reference_sem_number;
	rilascio.sem_op = +1;
	rilascio.sem_flg = 0;
}

/********** ATTACH ALLA CELLA **********/
/*
 *	Con questo metodo il processo taxi si attacca a una cella in modo casuale che trova
 *	tra le libere e quelle che non eccedono già la capacità.
 */
void attach(map *pointer_at_map) {
#ifdef MAPPA_VALORI_CASUALI

    random_cell();

    /* Entro in sezione critica */
    while (semop(taxi_sem_id, &accesso, 1) == -1) { /* Possibile loop infinito. Dipende dai controlli. */
    	random_cell();
    }
    /* Ci parcheggiamo */
    if (pointer_at_map->mappa[tmpx][tmpy].active_taxis <= pointer_at_map->mappa[tmpx][tmpy].taxi_capacity) {
    	x = tmpx;
    	y = tmpy;
    }
    semop(taxi_sem_id, &rilascio, 1);
    
    /* Verifico se la cella è 1 o 3 e prendo l'id della coda di messaggi. */
    if (pointer_at_map->mappa[x][y].cell_type == 1 || pointer_at_map->mappa[x][y].cell_type == 3) {
		msg_queue_of_cell_key = pointer_at_map->mappa[x][y].message_queue_key;
	}
	/* Ci attacchiamo alla coda di messaggi */
	msg_queue_of_cell = msgget(msg_queue_of_cell_key, 0);
	if (msg_queue_of_cell == -1){
		perror("Sono un processo Taxi: non riesco ad attaccarmi alla coda di messaggi della mia cella.");
	}
#endif
#ifndef MAPPA_VALORI_CASUALI	
		
	x = 2;
	y = 2;
	msg_queue_of_cell_key = pointer_at_map->mappa[x][y].message_queue_key;
	msg_queue_of_cell = msgget(msg_queue_of_cell_key, 0);
	if (msg_queue_of_cell == -1){
		perror("Sono un processo Taxi: non riesco ad attaccarmi alla coda di messaggi della mia cella.");
	}
#endif
}  
/********** MAIN **********/
/*
 *	All'interno della funzione main il processo deve prendere controllo del segmento di memoria
 *	condivisa dove ho la mappa e dei vari semafori che userà per i suoi metodi.
 */
int main(int argc, char *argv[])
{	
    int i,j,SO_HOLES=0;
    /* Prendo l'id e mi attacco al segmento */ 
    map_shm_id = atoi(argv[1]);
    pointer_at_map = shmat(map_shm_id, NULL, 0);
    if (pointer_at_map == NULL){
        perror("Processo Taxi: non riesco ad accedere alla mappa. Termino.");
        exit(EXIT_FAILURE);
    }

    /* Prendo visibilità dell'array di semafori Taxi*/
    for (i = 0; i < SO_HEIGHT; i ++) {
        for (j = 0; j < SO_WIDTH; j++){
            if (pointer_at_map->mappa[i][j].cell_type == 0) SO_HOLES++;
        }
    }
    taxi_sem_id = semget(TAXI_SEM_KEY, TAXI_SEM_ARRAY_DIM, SEM_FLG);
    if (taxi_sem_id == -1){
        perror("Processo Taxi: non riesco ad accedere al mio semaforo. Termino.");
        TEST_ERROR 
        exit(EXIT_FAILURE);
    }
    /* Chiamo il metodo attach */
    /*attach(pointer_at_map);*/

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
