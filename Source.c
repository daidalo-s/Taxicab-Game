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
#include <sys/time.h>
#include <signal.h>
#include "Function.h"

/****************** Prototipi ******************/
void set_handler(int signum, void(*function)(int));
void attach();
void destination_and_call();
void source_handler(int signum);
void message_handler(int signum);

/********** VARIABILI GLOBALI **********/	
map *pointer_at_map;
int map_shm_id = 0, source_sem_id = 0, msg_queue_of_cell_key = 0, message_queue_id = 0;
int x = 0, y = 0;
int start_sem_id;
struct sembuf accesso = { 0, -1, 0}; 
struct sembuf rilascio = { 0, +1, 0}; 
struct sembuf start = {0, -1, 0};

void set_handler(int signum, void(*function)(int)) {

	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	sa.sa_handler = function;
	sa.sa_flags = SA_SIGINFO;
	sigaction(signum, &sa, NULL);

}

/********** ATTACH ALLA CELLA **********/
/*
 *	Chiamando questa funzione il processo SO_SOURCE scorre la mappa alla ricerca di una cella
 *	con cell_type=1, prende il semaforo (unico per tutta la mappa) per la modifica del campo
 *	cell_type, in mutua esclusione modifica il campo cell_type a 3, preleva la key della coda
 *	di messaggi assegnata a quella cella, salva le coordinate della cella che si è preso.
 *	Rilascia il semaforo alla fine. 
 */
void attach() {
	
	int i,j, z = 0;

	/* Sezione critica */
	semop(source_sem_id, &accesso, 1);

	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			
			/* Cerco una cella SO_SOURCE e mi attacco */
			if (pointer_at_map->mappa[i][j].cell_type == 1) {
				z = 1;
				pointer_at_map->mappa[i][j].cell_type = 3;
				msg_queue_of_cell_key = pointer_at_map->mappa[i][j].message_queue_key;
				
				x = i;
				y = j;

				/* Prendo l'id della coda di messaggi della cella */
				message_queue_id = msgget(msg_queue_of_cell_key, 0600);
				if (message_queue_id == -1){
					perror("Processo Source: non riesco a collegarmi alla coda di messaggi della mia cella. Termino.");
					exit(EXIT_FAILURE);
				}
				break;
			}
		}
		if (z != 0){
			break;
		}
	}
	/* Rilascio la risorsa */	
	semop(source_sem_id, &rilascio, 1);
} 

/********** GENERAZIONE DI DESTINAZIONE CASUALE E MESSAGGI **********/
/*	
	Con la funzione rand estrae dei valori
	casuali per destination_x e destination_y verificando che il risultato non porti o a una cella
	hole o alla cella in cui il processo già si trova. A quel punto "confeziona" il messaggio 
	destination_string -che ha grandezza massima prevista di aaa,bbb- aggiungendo una virgola a 
	separazione di x e y. Imposta a questo punto il campo long del messaggio a 1 (che dovrà essere 
	lo stesso nei processi riceventi) e lo invia. 
*/
void destination_and_call() {

	int destination_x, destination_y;
	char str1[5], comma[] = {","}, str2[4];
	char destination_string[MESSAGE_WIDTH];
	message_queue cell_message_queue;

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
	strcat(destination_string, comma);
	sprintf(str2, "%d", x);
	strcat(str2, comma);
	strcat(destination_string, str2);
	sprintf(str2, "%d", y);
	strcat(destination_string, str2);
	
	/* Imposto i campi della struct message_queue */
	cell_message_queue.mtype = 1; /* Le richieste hanno long 1 */
	strcpy(cell_message_queue.message, destination_string);
	
	/* Invio il messaggio */
	if (msgsnd(message_queue_id, &cell_message_queue, MESSAGE_WIDTH, 0) < 0) {
		perror("Processo Source: errore, non riesco a mandare il messaggio");
		/* Lo facciamo terminare? */
	}
}

/* Terminazione su segnale del master dopo SO_DURATION*/
void source_handler(int signum) {

	/* Handler dopo SO_DURATION*/
	if (signum == SIGTERM) { 
		if (map_shm_id != 0) shmdt(pointer_at_map);
		kill(getpid(), SIGKILL);
	}

	/* Handler per ctrl c*/
	if (signum == SIGINT) {
		if (map_shm_id != 0) shmdt(pointer_at_map);
		kill(getpid(), SIGKILL);
	}
	
}

/* Handler per immissione di messaggi nella coda da segnale SIGUSR1*/
void message_handler(int signum) {	

	printf("Sono il source %i e ho ricevuto il segnale, immetto un messaggio nella mia coda \n", getpid());
	destination_and_call();

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
int main(int argc, char *argv[]) {
	
	struct timeval time;
	
	set_handler(SIGTERM, &source_handler);
	set_handler(SIGINT, &source_handler); 
	set_handler(SIGUSR1, &message_handler);
	
	gettimeofday(&time, NULL);
	srand((time.tv_sec * 1000) + (time.tv_usec));  
	
	/* Mi collego alla mappa */	
	map_shm_id = atoi(argv[1]);
	pointer_at_map = shmat(map_shm_id, NULL, 0);
	if (pointer_at_map == NULL) {
		perror("Processo Source: non riesco ad attaccarmi alla mappa. Termino. ");
		exit(EXIT_FAILURE);
	}
	
	/* Ottengo visibilità del semaforo a cui devo fare riferimento per l'assegnazione */
	source_sem_id = semget(SOURCE_SEM_KEY, 1, 0600);
	if (source_sem_id == -1){
		perror("Processo Source: non riesco a prendere il semaforo. Termino.");
		exit(EXIT_FAILURE);
	}

	/* Ottengo visibilità del semaforo a cui devo fare riferimento per il via */
	start_sem_id = semget(START_SEM_KEY, 1, 0600);
	if (start_sem_id == -1) {
		perror("Processo Source: non riesco a prendere il semaforo per il via. Termino ");
		exit(EXIT_FAILURE);
	}

	attach();
	
	/* Attendo il via dal master */
	semop(start_sem_id, &start, 1);
	
	while (1) { 	
 		sleep(3);
		destination_and_call();
	}

	return 0;
}
