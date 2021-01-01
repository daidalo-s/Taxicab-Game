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
#include <sys/msg.h>
#include "Map.h"
/********** Variabili globali **********/
map *pointer_at_map;
int shm_id, sem_id, msg_queue_id, x, y;
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
                msg_queue_id = pointer_at_map->mappa[i][j].message_queue;
                x = i;
                y = j;
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

/********** Generazione di destinazione e messaggi **********/
void destination_and_call(map *pointer_at_map) {
    int i,j,dimension_message, dimension_long;
    struct my_msgbuf msgp;
    char str1[4];
    /*char str2[4];*/
    char destination[13];

    /* Generare due coordinate tra le celle valide */
    srand(getpid());
    do { 
        i = rand() % ((SO_HEIGHT-1) - 0 + 1) + 0; 
        j = rand() % ((SO_WIDTH-1) - 0 + 1) + 0;
    } while (pointer_at_map->mappa[i][j].cell_type == 0 || (i == x && j == y));
    printf("Il valore di i e' %i \n", i);
    printf("Il valore di j e' %i \n", j);
    /* Immettere le coordinate nella coda di messaggi */
#if 0
    strcpy(str1, i);
    strcpy(str2, j);
    strcat(destination, str1);
    strcat(destination, str2);
#endif
#if 1
    sprintf(destination, "%d", i);
    sprintf(str1, "%d", j);
    strcat(destination, str1);
    printf("Stampo destination \n");
    printf("%s \n", destination);
#endif
#if 1
    msgp.mtype = 0; /* Le richieste hanno long 0 */
    strcpy(msgp.message, destination);
    dimension_message = sizeof(msgp);
    dimension_long = sizeof(long);
    printf("L'id della coda di messaggi in cui proverò a scrivere è %i \n", msg_queue_id);
    if (msgsnd(msg_queue_id, (void *) &msgp, SCEMO_CHI_LEGGE, 0) == -1) {
    	perror("DIo bastardo \n");
    };
#endif
}


/********** Main **********/
int main(int argc, char *argv[])
{	
	struct my_msgbuf msgp;
	int dimension_message = sizeof(msgp);
    int dimension_long = sizeof(long);
    sleep(5);
    /* Prendo l'indirizzo */ 
    shm_id = atoi(argv[1]);
    /* Mi attacco al segmento */
    pointer_at_map = shmat(shm_id, NULL, 0);
    /* Ottengo l'accesso al semaforo */
    sem_id = semget(SEM_KEY, 1, 0600);
#ifdef DEBUG
    printf("L'id del semaforo che ho in source è %i \n", sem_id);
#endif
    /* Cerco una cella SO_SOURCE e mi attacco */
    attach(pointer_at_map);
    destination_and_call(pointer_at_map);
#ifdef DEBUG
    printf("Sono un processo SO_SOURCE \n");
    printf("Il campo della cella 2.2 e': %i \n", pointer_at_map->mappa[2][2].cell_type);
#endif
#ifdef DEBUG_STAMPA_MAPPA  
    printf("Uso il metodo di stampa tradizionale \n");
    map_print(pointer_at_map);
#endif
    printf("L'id della coda di messaggi da cui proverò a leggere è %i \n", msg_queue_id);
    printf("Sono il processo source che proverà a ricevere il messaggio \n");
    msg_queue_id = pointer_at_map->mappa[2][2].message_queue;

    if (msgrcv(msg_queue_id, &msgp, (dimension_message - dimension_long), 0, 0) == -1) {
    	perror("Madonna tacchina \n ");
    };
    printf("Ora perdo un po' di tempo e poi esco \n");
    sleep(2);
    printf("Ho finito di dormire sono un processo Source \n");
    return 0;
}
