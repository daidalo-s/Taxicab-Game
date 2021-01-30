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
#include <sys/time.h>
#include "Function.h"


/********** VARIABILI GLOBALI **********/
/*  
 *	Deve accedere a: mappa, semaforo per assegnazione cella, coda di messaggi della cella.
 *	Deve ricordarsi di: dove si trova
 */	
map *pointer_at_map;
int map_shm_id = 0, source_sem_id = 0, msg_queue_of_cell_key = 0, message_queue_id = 0;
int x = 0, y = 0;
int start_sem_id;
struct sembuf accesso = { 0, -1, 0}; 
struct sembuf rilascio = { 0, +1, 0}; 
struct sembuf start = {0, -1, 0};

void map_print();

void set_handler(int signum, void(*function)(int)) {

	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	sa.sa_handler = function;
	sa.sa_flags = 0;
	sigaction(signum, &sa, NULL);

}

/********** ATTACH ALLA CELLA **********/
/*
 *	Chiamando questo metodo il processo SO_SOURCE scorre la mappa alla ricerca di una cella
 *	con cell_type=1, prende il semaforo (unico per tutta la mappa) per la modifica del campo
 *	cell_type, in mutua esclusione modifica il campo cell_type a 3, preleva la key della coda
 *	di messaggi assegnata a quella cella, salva le coordinate della cella che si è preso.
 *	Rilascia il semaforo alla fine. 
 */

void attach() {
	
	int i,j, z = 0;

	/* Sezione critica */
	semop(source_sem_id, &accesso, 1);
	TEST_ERROR

	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
			
			/* Cerco una cella SO_SOURCE e mi attacco */
			if (pointer_at_map->mappa[i][j].cell_type == 1){
				TEST_ERROR
				z = 1;
				pointer_at_map->mappa[i][j].cell_type = 3;
				msg_queue_of_cell_key = pointer_at_map->mappa[i][j].message_queue_key;
				
				printf("Ho preso id %x \n", msg_queue_of_cell_key);
				
				x = i;
				y = j;
				break;
			}
		}
		if (z != 0){
			break;
		}
	}
	/* Rilascio la risorsa */	
	semop(source_sem_id, &rilascio, 1);
	TEST_ERROR

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
void destination_and_call() {

	int destination_x, destination_y;
	char str1[5], comma[] = {","};
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
	
	/* Invio il messaggio */
	if (msgsnd(message_queue_id, &cell_message_queue, MESSAGE_WIDTH, 0) < 0) {
		perror("Processo Source: errore, non riesco a mandare il messaggio");
		/* Lo facciamo terminare? */
	}
}

/* Terminazione su segnale del master dopo SO_DURATION*/
void source_handler (int signum) {

	/* Handler dopo SO_DURATION*/
	if (signum == SIGTERM) { 
		printf("SOURCE Ricevo il segnale SIGTERM\n");
		if (map_shm_id != 0) shmctl(map_shm_id, IPC_RMID, NULL);
		if (source_sem_id != 0) semctl(source_sem_id, 0, IPC_RMID);
		if (start_sem_id != 0) semctl(start_sem_id, 0, IPC_RMID);
		kill(getpid(), SIGKILL);
	}

	/* Handler per ctrl c*/
	if (signum == SIGINT) {
		printf("SOURCE Ricevo segnale ctrl c\n");
		if (map_shm_id != 0) shmctl(map_shm_id, IPC_RMID, NULL);
		if (source_sem_id != 0) semctl(source_sem_id, 0, IPC_RMID);
		if (start_sem_id != 0) semctl(start_sem_id, 0, IPC_RMID);
		kill(getpid(), SIGKILL);
	}	

}

/* Handler immissione messaggi nella coda da segnale SIGUSR1*/
void message_handler (int signum) {	

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
int main(int argc, char *argv[])
{
	
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

#if 1	
	
	printf("Sono il processo %i che esegue \n", getpid());
	
	attach();
	
	/*
	printf("Sono la cella source in posizione x %i y %i \n", x, y); 
	printf("STAMPO LE MIE INFORMAZIONI: \n");
	printf("Sono in posizione x %i y %i \n", x, y); 
	printf("Id mappa %i , Id semaforo mutex %i , Key coda di messaggi della mia cella %x \n", map_shm_id, source_sem_id, msg_queue_of_cell_key);
	*/
	
	/* DOBBIAMO CHIAMARLA DOPO UNA RICEZIONE DI UN SEGNALE DA TERMINALE */
	/*
	printf("Ti lascio tempo per mandarmi segnali \n");
	sleep(10);
	*/
	
	/* Attendo il via dal master */
	printf("Aspetto il via dal master \n");
	semop(start_sem_id, &start, 1);
	
	while (1) { 	
		/* da cambiare */
 		sleep(2);
		destination_and_call();
	}

#ifdef DEBUG
	printf("Sono un processo SO_SOURCE \n");
	printf("Il campo della cella 2.2 e': %i \n", pointer_at_map->mappa[2][2].cell_type);
#endif

#ifdef DEBUG_STAMPA_MAPPA  
	printf("Uso il metodo di stampa tradizionale \n");
	map_print();
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
void map_print() {
	int i, j;
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			printf ("%i ", pointer_at_map->mappa[i][j].cell_type);
		}
		printf("\n");
	}
}
