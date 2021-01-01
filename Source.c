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
message_queue cell_message_queue;
int shm_id, sem_id, msg_queue_key, message_queue_id, x, y;
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
    int i,j;
    for (i = 0; i < SO_HEIGHT; i++){
        for (j = 0; j < SO_WIDTH; j++){
            if (pointer_at_map->mappa[i][j].cell_type == 1){
                /* Sezione critica */
                printf("Fino a prima della semop arrivo \n");
                semop(sem_id, &accesso, 1);

                pointer_at_map->mappa[i][j].cell_type = 3;
                msg_queue_key = pointer_at_map->mappa[i][j].message_queue;
                
                x = i;
                y = j;
                
                /* Rilascio la risorsa */
                semop(sem_id, &rilascio, 1);
            }
        }
    }
} 

/********** Generazione di destinazione e messaggi **********/
void destination_and_call(map *pointer_at_map) {
    
    int destination_x, destination_y, message_queue_id;
    char str1[5], comma[] = {","};
    char destination_string[MESSAGE_WIDTH];

    /* Generare due coordinate tra le celle valide */
    srand(getpid());
    do { 
        destination_x = rand() % ((SO_HEIGHT-1) - 0 + 1) + 0; 
        destination_y = rand() % ((SO_WIDTH-1) - 0 + 1) + 0;
    } while (pointer_at_map->mappa[destination_x][destination_y].cell_type == 0 || (destination_x == x && destination_y == y));
    printf("Il valore di i e' %i \n", destination_x);
    printf("Il valore di j e' %i \n", destination_y);
    /* Immettere le coordinate nella coda di messaggi */
#if 1
    sprintf(destination_string, "%d", destination_x);
    strcat(destination_string, comma);
    sprintf(str1, "%d", destination_y);
    strcat(destination_string, str1);
    printf("Stampo destination \n");
    printf("%s \n", destination_string);
#endif
    cell_message_queue.mtype = 1; /* Le richieste hanno long 1 */
    strcpy(cell_message_queue.message, destination_string);
    printf("%s \n", cell_message_queue.message);
    message_queue_id = msgget(msg_queue_key, 0);
    printf("L'id della coda di messaggi in cui proverò a scrivere è %i \n", msg_queue_key);
    if (msgsnd(message_queue_id, &cell_message_queue, MESSAGE_WIDTH, 0) < 0) {
    	perror("Errore non riesco a mandare il messaggio");
    }
}


/********** Main **********/
int main(int argc, char *argv[])
{
    sleep(2);
    /* Prendo l'indirizzo */ 
    shm_id = atoi(argv[1]);
    /* Mi attacco al segmento */
    pointer_at_map = shmat(shm_id, NULL, 0);
    /* Ottengo l'accesso al semaforo */
    sem_id = semget(SEM_KEY, 1, 0600);
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
    printf("L'id della coda di messaggi da cui proverò a leggere è %i \n", msg_queue_key);
    printf("Sono il processo source che proverà a ricevere il messaggio \n");
    msg_queue_key = pointer_at_map->mappa[2][2].message_queue;
    message_queue_id = msgget(msg_queue_key, 0);
    if (msgrcv(message_queue_id, &cell_message_queue, MESSAGE_WIDTH, 0, 0) < 0) {
        perror("Errore non riesco a ricevere il messaggio\n ");
    };
    printf("Ho ricevuto il messaggio %s \n", cell_message_queue.message);
    printf("Ora perdo un po' di tempo e poi esco \n");
    sleep(2);
    printf("Ho finito di dormire sono un processo Source \n");
    return 0;
}
