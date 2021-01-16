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


/********** VARIABILI GLOBALI **********/
/*  
 *	Deve accedere a: mappa, semaforo per assegnazione cella, coda di messaggi della cella.
 *	Deve ricordarsi di: dove si trova
 */	
map *pointer_at_map;
message_queue cell_message_queue;
int map_shm_id = 0, source_sem_id = 0, msg_queue_of_cell_key = 0, message_queue_id = 0;
int x = 0, y = 0;
struct sembuf accesso = { 0, -1, 0}; 
struct sembuf rilascio = { 0, +1, 0}; 
/*
int * puntatore_alla_mappa = &map_shm_id;
int * puntatore_a_source_sem_id = &source_sem_id;
int * puntatore_a_msg_queue_of_cell_key = &msg_queue_of_cell_key;
int * puntatore_a_msg_queue_id = &message_queue_id;
*/



void map_print(map *pointer_at_map);


/********** ATTACH ALLA CELLA **********/
/*
 *	Chiamando questo metodo il processo SO_SOURCE scorre la mappa alla ricerca di una cella
 *	con cell_type=1, prende il semaforo (unico per tutta la mappa) per la modifica del campo
 *	cell_type, in mutua esclusione modifica il campo cell_type a 3, preleva la key della coda
 *	di messaggi assegnata a quella cella, salva le coordinate della cella che si è preso.
 *	Rilascia il semaforo alla fine. 
 */

void attach(map *pointer_at_map) {
	
	int i,j;

	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
				
			if (pointer_at_map->mappa[i][j].cell_type == 1){
				TEST_ERROR
				
				pointer_at_map->mappa[i][j].cell_type = 3;
				msg_queue_of_cell_key = pointer_at_map->mappa[i][j].message_queue_key;
				
				printf("Ho preso id %x \n", msg_queue_of_cell_key);
				
				x = i;
				y = j;
				
			}
			/*
			printf("Stampo la mappa da attach \n");
			map_print(pointer_at_map);
			*/
		}
	}
} 

/********** GENERAZIONE DI DESTINAZIONE CASUALE E MESSAGGI **********/
/*	
	Chiamando questo metodo il processo SO_SOURCE genera due coordinate di destinazione
	che salva dentro destination_x e destination_y. Con la funzione rand estrae dei valori
	casuali per destination_x e destination_y verificando che il risultato non porti o a una cella
	hole o alla cella in cui il processo già si trova. A quel punto "confeziona" il messaggio 
	destination_string -che ha grandezza massima prevista di aaa,bbb- aggiungendo una virgola a 
	separazione di x e y. Imposta a questo punto il campo long del messaggio a 1 (che dovrà essere 
	lo stesso nei processi riceventi) e lo invia. 
	L'INVIO DOVRÀ ESSERE PERIODICO.
	*/
void destination_and_call(map *pointer_at_map) {

	int destination_x, destination_y, message_queue_id;
	char str1[5], comma[] = {","};
	char destination_string[MESSAGE_WIDTH];

	/* Generare due coordinate tra le celle valide */
	do { 
		destination_x = rand() % ((SO_HEIGHT-1) - 0 + 1) + 0; 
		destination_y = rand() % ((SO_WIDTH-1) - 0 + 1) + 0;
	} while (pointer_at_map->mappa[destination_x][destination_y].cell_type == 0 || (destination_x == x && destination_y == y));

	/* Preparo il messaggio */
	sprintf(destination_string, "%d", destination_x);
	strcat(destination_string, comma);
	sprintf(str1, "%d", destination_y);
	strcat(destination_string, str1);
	
	/* Imposto i campi della struct message_queue */
	cell_message_queue.mtype = 1; /* Le richieste hanno long 1 */
	strcpy(cell_message_queue.message, destination_string);
	printf("Il messaggio che manderò è %s \n", cell_message_queue.message);

	/* Prendo l'id della coda di messaggi e mando */
	message_queue_id = msgget(msg_queue_of_cell_key, 0600);
	if (message_queue_id == -1){
		perror("Processo Source: non riesco a collegarmi alla coda di messaggi della mia cella. Termino.");
		exit(EXIT_FAILURE);
	}
	/* DA FARE IN MODO PERIODICO */
	if (msgsnd(message_queue_id, &cell_message_queue, MESSAGE_WIDTH, 0) < 0) {
		perror("Processo Source: errore, non riesco a mandare il messaggio");
		/* Lo facciamo terminare? */
	}
}


/********** MAIN **********/
/*
   All'interno della funzione main il processo SO_SOURCE come prima cosa imposta il puntatore
   alla mappa in memoria convidisa con l'id gli viene passato da Master.c come argomento alla
   execve. A questo punto si "collega" anche al semaforo per l'assegnamento delle celle SOURCE
   con la chiamata ad attach(). 
   Quando arriva il segnale del master di "go" chiama destination_and_call per cominciare a 
   generare richieste. 
   */
int main(int argc, char *argv[])
{
	srand(time(NULL)); /* Forse va bene forse no */

	/* Mi collego alla mappa */	
	map_shm_id = atoi(argv[1]);
	pointer_at_map = shmat(map_shm_id, NULL, 0);
	if (pointer_at_map == NULL) {
		perror("Processo Source: non riesco ad attaccarmi alla mappa. Termino. ");
		exit(EXIT_FAILURE);
	}
	
	/* Ottengo visibilità del semaforo a cui devo fare riferimento */
	source_sem_id = semget(SOURCE_SEM_KEY, 1, 0600);
	if (source_sem_id == -1){
		perror("Processo Source: non riesco a prendere il semaforo. Termino.");
		exit(EXIT_FAILURE);
	}

#if 1	
	/* Cerco una cella SO_SOURCE e mi attacco */
	/* Sezione critica */
	
	semop(source_sem_id, &accesso, 1);
	TEST_ERROR
	
	attach(pointer_at_map);
	/* printf("Sono la cella source in posizione x %i y %i \n", x, y); */
	/* Rilascio la risorsa */
	
	semop(source_sem_id, &rilascio, 1);
	TEST_ERROR
	
	printf("STAMPO LE MIE INFORMAZIONI: \n");
	printf("Sono in posizione x %i y %i \n", x, y); 
	printf("Id mappa %i , Id semaforo mutex %i , Key coda di messaggi della mia cella %x \n", map_shm_id, source_sem_id, msg_queue_of_cell_key);


	/* DOBBIAMO CHIAMARLA DOPO UNA RICEZIONE DI UN SEGNALE DA TERMINALE */
	destination_and_call(pointer_at_map); 

	/* Uccido i semafori che creo per sbaglio DA RIMUOVERE QUANDO RISOLTO */
	if (msgctl(message_queue_id , IPC_RMID , NULL) == -1){
		perror("DIo cannone ");
	} 

#ifdef DEBUG
	printf("Sono un processo SO_SOURCE \n");
	printf("Il campo della cella 2.2 e': %i \n", pointer_at_map->mappa[2][2].cell_type);
#endif

#ifdef DEBUG_STAMPA_MAPPA  
	printf("Uso il metodo di stampa tradizionale \n");
	map_print(pointer_at_map);
#endif

#ifdef DEBUG_RICEZIONE_MESSAGGIO
	printf("L'id della coda di messaggi da cui proverò a leggere è %i \n", msg_queue_of_cell_key);
	printf("Sono il processo source che proverà a ricevere il messaggio \n");
	msg_queue_of_cell_key = pointer_at_map->mappa[2][2].message_queue_key;
	message_queue_id = msgget(msg_queue_of_cell_key, 0);
	if (msgrcv(message_queue_id, &cell_message_queue, MESSAGE_WIDTH, 0, 0) < 0) {
		perror("Errore non riesco a ricevere il messaggio\n ");
	};
	printf("Ho ricevuto il messaggio %s \n", cell_message_queue.message);
#endif 

#endif
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
