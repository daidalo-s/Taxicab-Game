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

/********** VARIABILI GLOBALI **********/
/*  
 *	Deve accedere a: mappa in memoria condivisa, coda di messaggi della cella
 *      che sceglie, semaforo per active_taxis, semaforo per crossings
 *	Deve ricordarsi di: dove si trova per scegliere il percorso.
 */	
int x, y;
int random_coordinates[2];
int tmpx, tmpy;
int previous_x, previous_y;
int msg_queue_of_cell_key, msg_queue_of_cell, map_shm_id, taxi_sem_id, adjacency_matrix_shm_id;
int num_of_vertices;
int element_counter;
int length_of_path;
int start_sem_id;
int SO_TIMEOUT;
int ongoing_trip = 0;
int * distance;
int * predecessor;
int * visited;
int * path_to_follow;
int ** pointer_at_adjacency_matrix;
struct sembuf accesso;
struct sembuf rilascio;
struct sembuf start = {0, -1, 0};
message_queue cell_message_queue;    
map *pointer_at_map; 

/********** PROTOTIPI **********/
void random_cell();
void attach();
void receive_and_go();
void find_path(int start_vertex, int destination_vertex);
void create_index(void **m, int rows, int cols, size_t sizeElement);
void map_print();

void set_handler(int signum, void(*function)(int)) {

	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	sa.sa_handler = function;
	sa.sa_flags = SA_SIGINFO;
	sigaction(signum, &sa, NULL);

}

/* Mi occupo di impostare la struct di accesso e rilascio in base a dove sono */
void setting_sem_struct (int x, int y) {

	/* Accesso */
	accesso.sem_num = pointer_at_map->mappa[x][y].reference_sem_number; 
	accesso.sem_op = -1;
	accesso.sem_flg = SEM_UNDO;

	/* Rilascio */
	rilascio.sem_num = pointer_at_map->mappa[x][y].reference_sem_number;
	rilascio.sem_op = +1;
	rilascio.sem_flg = 0;	
} 

void random_cell() {
	/* Possibile loop infinito, dipende dai controlli */
	/* Tiro a caso valori finche' trovo una cella non hole*/
	do {
		random_coordinates[0] = rand() % ((SO_HEIGHT-1) - 0 + 1) + 0; /* x == i */
		random_coordinates[1] = rand() % ((SO_WIDTH-1) - 0 + 1) + 0; /* y == j*/
	} while (pointer_at_map->mappa[random_coordinates[0]][random_coordinates[1]].cell_type == 0);
	
	tmpx = random_coordinates[0]; /* == i */
	tmpy = random_coordinates[1]; /* == j */
	
	/* Non posso metterla nella funzione perché non deve essere bloccante */ 
	/* Accesso */
	accesso.sem_num = pointer_at_map->mappa[tmpx][tmpy].reference_sem_number; 
	accesso.sem_op = -1;
	accesso.sem_flg = IPC_NOWAIT | SEM_UNDO;

	/* Rilascio */
	rilascio.sem_num = pointer_at_map->mappa[tmpx][tmpy].reference_sem_number;
	rilascio.sem_op = +1;
	rilascio.sem_flg = 0;	

}

/********** ATTACH ALLA CELLA **********/
/*
 *	Con questo metodo il processo taxi si attacca a una cella in modo casuale che trova
 *	tra le libere e quelle che non eccedono già la capacità.
 *  Viene eseguito solo all'inizio 
 */
void attach() {
#ifdef MAPPA_VALORI_CASUALI

	random_cell();
	/* Entro in sezione critica */
	while (semop(taxi_sem_id, &accesso, 1) == -1) { /* Possibile loop infinito. Dipende dai controlli. */
		random_cell();
	}

	/* Ci parcheggiamo. Quindi siamo sicuri di poterlo fare? */
	if (pointer_at_map->mappa[tmpx][tmpy].active_taxis < pointer_at_map->mappa[tmpx][tmpy].taxi_capacity) {
		x = tmpx;
		y = tmpy;
		pointer_at_map->mappa[x][y].active_taxis++;
	} else {
		/* In teoria dovrebbe sparire */
		printf("SONO UN TAXI CHE HA SBAGLIATO TUTTO NELLA VITA \n");
	}
#endif

}  

void receive_and_find_path() {
	
	int x_destination = 0, y_destination = 0;

	int start_vertex = 0, destination_vertex = 0;
	
	printf("ARGOMENTI PER LA msgrcv \n");	
	printf("%i \n", msg_queue_of_cell);
	printf("%i \n", MESSAGE_WIDTH);

	while (msgrcv(msg_queue_of_cell, &cell_message_queue, MESSAGE_WIDTH, 1, 0) < 0){
		perror("Errore non riesco a ricevere il messaggio\n ");
		TEST_ERROR
	}

	printf("Ho ricevuto il messaggio %s \n", cell_message_queue.message); 
	x_destination = atoi(&cell_message_queue.message[0]);
	y_destination = atoi(&cell_message_queue.message[2]);
	
	printf("La x_destination e' % i \n", x_destination);
	printf("La y_destination e' %i \n", y_destination);
	printf("Il vertex_number della destination e' %i \n", pointer_at_map->mappa[x_destination][y_destination].vertex_number);
	
	/* Devo chiamare dijkstra: ho bisogno di passare il vertice in cui mi trovo e il vertice in cui voglio andare */
	start_vertex = pointer_at_map->mappa[x][y].vertex_number; /* La cella in cui sono */
	destination_vertex = pointer_at_map->mappa[x_destination][y_destination].vertex_number; /* La cella in cui voglio andare*/
	printf("Cerco un percorso \n");
	find_path(start_vertex, destination_vertex);
	/* Accedendo a path to follow il primo elemento è la prossima cella in cui andare */

}


void find_path(int start_vertex, int destination_vertex) {

	int count = 0, min_distance = 0, next_node = 0, i = 0, j = 0, tmp_int = 0;
	
	printf("Creo l'array per le distanze, sono %i \n", getpid());
	/* Creo l'array per contenere la distanza, lo faccio ad ogni nuovo viaggio */
	distance = malloc(num_of_vertices * sizeof(int));
	if (distance == NULL) {
		perror("1");
	}

	printf("Creo l'array per i nodi precedenti, sono %i \n", getpid());
	/* Creo l'array per salvare la provenienza, lo faccio ad ogni nuovo viaggio */
	predecessor = malloc(num_of_vertices * sizeof(int));
	if (predecessor == NULL) {
		perror("2");
	}

	printf("Creo l'array per i nodi visitati, sono %i \n", getpid());
	/* Creo l'array per salvare i vertici visitati, lo faccio ad ogni nuovo viaggio */
	visited = malloc(num_of_vertices * sizeof(int));
	if (visited == NULL) {
		perror("3");
	}
	
	printf("Inizializzo i tre array, sono %i \n", getpid());
	/* Inizializzo i tre array */
	for (i = 0; i < num_of_vertices; i++){
		distance[i] = pointer_at_adjacency_matrix[start_vertex][i];
		predecessor[i] = start_vertex;
		visited[i] = 0;
	}

	distance[start_vertex] = 0;
	visited[start_vertex] = 1;
	count = 1;

	printf("Entro nel while, sono %i  \n", getpid());
	while(count < num_of_vertices-1){
		
		min_distance = INFINITY;
		
		printf("Trovo il nodo alla distanza minima, sono %i \n", getpid());
		/* Trovo il nodo alla distanza minima */
		for (i = 0; i < num_of_vertices; i++) { 
			if (distance[i] < min_distance && !visited[i]) {
				min_distance = distance[i];
				next_node = i;
			}
		}
		
		printf("Cerco un percorso migliore, sono %i \n", getpid());
		/* Continuo l'esplorazione alla ricerca di un path migliore */
		visited[next_node] = 1;

		for (i = 0; i < num_of_vertices; i++) {  
			if (!visited[i]) 

				if (min_distance + pointer_at_adjacency_matrix[next_node][i] < distance[i]){
					distance[i] = min_distance + pointer_at_adjacency_matrix[next_node][i];
					predecessor[i] = next_node;
				}	
		}			
		count++;
	}

	printf("Creo l'array dove salvare il percorso, sono %i \n", getpid());
	/* Creo l'array dove salvo il percorso che devo seguire */
	tmp_int = distance[destination_vertex] - 1;

	/* Potrebbe essere eliminato, element counter fa lo stesso lavoro */
	length_of_path = tmp_int;
	path_to_follow = malloc((tmp_int+1) * sizeof(int));
	element_counter = 0;

	printf("Copio il path, sono %i \n", getpid());
	do {
		path_to_follow[tmp_int] = destination_vertex;
		destination_vertex = predecessor[destination_vertex];
		tmp_int --;
		element_counter++;
	} while (destination_vertex != start_vertex);
	/* printf("TEST METODO DIJKSTRA: STAMPO IL PATH PIU BREVE \n"); */
	/* Qua no */ 
	for (j = 0; j < element_counter; j++){
		printf("%i \t", path_to_follow[j]);
	}
	printf("\n");
	printf("Libero le risorse, sono %i \n",getpid() );
	free(visited);
	visited = NULL;

	free(predecessor);
	predecessor = NULL;
	
	free(distance);
	distance = NULL;
}


void move() {
	
	int i = 0, j = 0, k = 0, next_vertex = 0, stop = 0;
	
	/* La struct dove salvo il tempo */
	struct timespec ts; 
	printf("Ho finito l'inizializzaione di move, sono %i \n", getpid());
	
	/* Finche' sono all'interno dell'array del percorso */
	while (k <= length_of_path) {
		
		printf("Entro nel ciclo di move, sono %i \n", getpid());
		stop = 0;

		/* Prendo il tempo della cella in cui mi trovo */
		
		ts.tv_sec = 0;
		ts.tv_nsec = pointer_at_map->mappa[x][y].travel_time;
		printf("SOno %i, i nanosecondi per cui devo dormire sono % i \n", getpid(), pointer_at_map->mappa[x][y].travel_time);
		
		printf("Ho impostato la struct per la nanosleep, sono %i \n", getpid());

		/* Dormo il tempo giusto */
		if (nanosleep(&ts, NULL) == -1){
			perror("Non riesco a dormire");
		}

		pointer_at_map->mappa[x][y].crossings++;
		
		printf("Ho preso il prossimo vertice, sono %i \n", getpid());
		/* Prendo il prossimo vertice */
		next_vertex = path_to_follow[k];
		
		/* Alarm per la terminazione, devo salvare la cella in cui mi trovavo */
		previous_x = x;
		previous_y = y;

		alarm(SO_TIMEOUT);

		printf("Entro nel ciclo per trovare il prossimo vertice, sono %i \n", getpid());
		/* Aggiorno i valori di x e y: MIGLIORABILE DIVIDENDO LA MAPPA IN QUADRANTI */
		for (i = 0; i < SO_HEIGHT; i++){
			for (j = 0; j < SO_WIDTH; j++){
				if (pointer_at_map->mappa[i][j].vertex_number == next_vertex) {
					/* Esco dalla cella in cui mi trovavo */
					pointer_at_map->mappa[x][y].active_taxis = pointer_at_map->mappa[x][y].active_taxis - 1;
					semop(taxi_sem_id, &rilascio, 1);

					setting_sem_struct(i, j);

					semop(taxi_sem_id, &accesso, 1);

					x = i;
					y = j;
					
					pointer_at_map->mappa[x][y].active_taxis++;
					
					stop = 1;
					break;
				}
			}
			if (stop) {
				break;
			}
		}
		printf("Mi sono mosso di una cella, sono %i \n", getpid());
		k++;
	}

}

void create_index(void **m, int rows, int cols, size_t sizeElement){
	int i;  
	size_t sizeRow = cols * sizeElement;
	m[0] = m+rows;
	for(i=1; i<rows; i++){      
		m[i] = ((u_int8_t*)m[i-1]+sizeRow);
	}
}

void kill_all() {

	/* Array */
	if (distance != NULL) free (distance);
	if (predecessor != NULL) free (predecessor);
	if (visited != NULL) free (visited);
	if (path_to_follow != NULL) free (path_to_follow);

	/* Memoria condivisa */
	if (map_shm_id != 0) shmctl(map_shm_id, IPC_RMID, NULL);
	if (adjacency_matrix_shm_id != 0) shmctl(adjacency_matrix_shm_id, IPC_RMID, NULL);
	if (taxi_sem_id != 0) semctl(taxi_sem_id, 0, IPC_RMID);
	if (start_sem_id != 0) semctl(start_sem_id, 0, IPC_RMID);
}

void taxi_handler (int signum) {
	
	/* Handler dopo SO_TIMEOUT */
	if (signum == SIGTERM) { 
		printf("TAXI Ricevo il segnale SIGTERM\n");
		kill_all(); 
		kill(getpid(), SIGKILL);
	}
	
	/* Handler dopo ctrl c*/
	if (signum == SIGINT) {
		printf("TAXI Ricevo il segnale SIGINT\n"); 
		kill_all();
		kill(getpid(), SIGKILL);
	}

	/* Alarm SO_TIMEOUT */
	if (signum == SIGALRM) {

		printf("Ho ricevuto il segnale TIMEOUT \n");
		if (previous_x == x && previous_y == y) {
			/* E se non stavo effettuando una corsa? */
			if (ongoing_trip) { 	
				pointer_at_map->mappa[x][y].aborted_trip++;
			}
			pointer_at_map->mappa[x][y].active_taxis--;
			kill_all();
			kill(getppid(), SIGUSR2);
			pause();
		} 
	}	
}


/********** MAIN **********/
/*
 *	All'interno della funzione main il processo deve prendere controllo del segmento di memoria
 *	condivisa dove ho la mappa e dei vari semafori che userà per i suoi metodi.
 */
int main(int argc, char *argv[])
{	
	
	int i,j,SO_HOLES=0, source;

	int first_free_source;  

	int mph, mpw;

	int stop = 0;

	/* Inizializzazione rand */
	struct timeval time;
	gettimeofday(&time, NULL);
    srand((time.tv_sec * 1000) + (time.tv_usec));  
	
	/* Impostiamo gli handler per i segnali che gestiamo */
	set_handler(SIGINT, &taxi_handler);
	set_handler(SIGTERM, &taxi_handler);
	set_handler(SIGALRM, &taxi_handler);

	/* Calcolo il numero di holes per calcolare la dimensione della matrice adiacente */
	for (i = 0; i < SO_HEIGHT; i ++) {
		for (j = 0; j < SO_WIDTH; j++){
			if (pointer_at_map->mappa[i][j].cell_type == 0) SO_HOLES++;
		}
	}

	/* Prendo l'id della mappa e mi attacco al segmento */ 
	map_shm_id = atoi(argv[1]);
	pointer_at_map = shmat(map_shm_id, NULL, 0);
	if (pointer_at_map == NULL){
		perror("Processo Taxi: non riesco ad accedere alla mappa. Termino.");
		exit(EXIT_FAILURE);
	}

	/* Prendo l'id della matrice adiacente e mi attacco */
	adjacency_matrix_shm_id = atoi(argv[2]);
	pointer_at_adjacency_matrix = (int **)shmat(adjacency_matrix_shm_id, NULL, 0);
	if (pointer_at_adjacency_matrix == NULL){
		perror("Processo Taxi: non riesco ad accedere alla matrice adiacente. Termino.");
		exit(EXIT_FAILURE);
	}	
	num_of_vertices = (SO_WIDTH*SO_HEIGHT)-SO_HOLES;
	printf("num_of_vertices %i \n", num_of_vertices);
	create_index((void*)pointer_at_adjacency_matrix, num_of_vertices, num_of_vertices, sizeof(int)); 

	/* Leggo SO_TIMEOUT */
	SO_TIMEOUT = atoi(argv[3]);

	/* Prendo visibilita dell'array di semafori per l'accesso alle celle */
	taxi_sem_id = semget(TAXI_SEM_KEY, TAXI_SEM_ARRAY_DIM, SEM_FLG);
	if (taxi_sem_id == -1){
		perror("Processo Taxi: non riesco ad accedere al mio semaforo. Termino.");
		exit(EXIT_FAILURE);
	}

	/* Prendo visibilità del semaforo per il via */
	start_sem_id = semget(START_SEM_KEY, 1, 0600);
	if (start_sem_id == -1){
		perror("Processo Taxi: non riesco a prendere visibilità de semaforo per il via. Termino. ");
		exit(EXIT_FAILURE);
	}

#if 1

	/*
	printf("Stampo la mappa secondo la capacità dei taxi \n");
	for(i = 0; i < SO_HEIGHT; i++){
		for(j = 0; j < SO_WIDTH; j++){
			printf("%i  ", pointer_at_map->mappa[i][j].taxi_capacity);
		}
		printf("\n");
	}
	*/

	/* Ci posizionamo a caso sulla mappa */
	attach();
	
	/* printf("Aspetto il via dal master \n"); */
	semop(start_sem_id, &start, 1);

	/*
	printf("Stampo la mappa secondo il numero di taxi attivi \n");
	for(i = 0; i < SO_HEIGHT; i++){
		for(j = 0; j < SO_WIDTH; j++){
			printf("%i  ", pointer_at_map->mappa[i][j].active_taxis);
		}
		printf("\n");
	}
	*/

	/* 
	printf("SONO UN PROCESSO TAXI: STAMPO LA MATRICE ADIACENTE \n");
	   for (i = 0; i < num_of_vertices; i++){
	   for (j = 0; j < num_of_vertices; j++){
	   	printf("%d ", pointer_at_adjacency_matrix[i][j]); 
	   }
	   printf("\n");
	   }
	*/

	/* ------------ TAXI INIZIALIZZATO --------------- */
	/*
	printf("Provo a stampare la mappa \n");
	map_print();
	*/
	while (1) {

		source = stop = ongoing_trip = 0;
		
		printf("Mi trovo nella cella con x %i e y %i e cell_type %i e vertex_number %i \n", x, y, pointer_at_map->mappa[x][y].cell_type, pointer_at_map->mappa[x][y].vertex_number);
		
		/* Devo capire in quale tipologia di cella mi trovo */
		printf("controllo che la cella sia source \n");
		if (pointer_at_map->mappa[x][y].cell_type == 3) {
			source = 1;
		}	
		printf("Il valore di source e' %i \n", source);
		
		/* Biforcazione in base a dove sono */
		if (source == 1) {

			/* Prende la key per la coda di messaggi */
			msg_queue_of_cell_key = pointer_at_map->mappa[x][y].message_queue_key;		
			
			/* Ci attacchiamo alla coda di messaggi */
			printf("LA KEY A CUI MI ATTACCO %i \n", msg_queue_of_cell_key);
			msg_queue_of_cell = msgget(msg_queue_of_cell_key, 0);
			if (msg_queue_of_cell == -1){
				perror("Sono un processo Taxi: non riesco ad attaccarmi alla coda di messaggi della mia cella.");
			}
			
			/* Leggo un messaggio e calcolo il percorso */
			receive_and_find_path();

			/* Controllo per eventuali errori DA TOGLIERE */
			for (i = 0; i <= length_of_path; i++){
				if (path_to_follow[i] < 0) printf("SPERO DI NON VEDERLO MAI \n");
			}

			ongoing_trip = 1;

			printf("Parto da una Source \n");
			
			/* Parto */
			move();

			/* Una volta che esco dalla move dovrei essere arrivato nella cella di destinazione */
			/* Verifico di essere arrivato a destinazione */
			if (pointer_at_map->mappa[x][y].vertex_number == path_to_follow[length_of_path]) {
				printf("Sono giunto a destinazione \n");
				pointer_at_map->mappa[x][y].completed_trip++;
				
				/* Libero l'array contente il path */
				free(path_to_follow);
				path_to_follow = NULL;  
			} else {
				printf("Qualcosa non ha funzionato nel movimento da una Source, non sono arrivato a destinazione. Termino.\n");
				exit(EXIT_FAILURE);
			}
			

		} else {

			mph = SO_HEIGHT / 2; 
			mpw = SO_WIDTH / 2;
			
			/* q1 */
			if (x < mph && y < mpw){
				for(i = 0; i < SO_HEIGHT; i++){
					for(j = 0; j < SO_WIDTH; j++){
						if (pointer_at_map->mappa[i][j].cell_type == 3 && 
							(pointer_at_map->mappa[i][j].active_taxis < pointer_at_map->mappa[i][j].taxi_capacity)) {
							first_free_source = pointer_at_map->mappa[i][j].vertex_number;
							printf("L'ho trovata, e' il numero %i . \n", first_free_source);
							stop = 1;
							break; 
						}
					}
					if (stop) {
						break;
					}
				    
				}
			} else if (x < mph && y >= mpw){
				/* q2 */
				for(i = 0; i < SO_HEIGHT; i++){
					for(j = SO_WIDTH-1; j >= 0; j--){
						if (pointer_at_map->mappa[i][j].cell_type == 3 && 
							(pointer_at_map->mappa[i][j].active_taxis < pointer_at_map->mappa[i][j].taxi_capacity)) {
							first_free_source = pointer_at_map->mappa[i][j].vertex_number;
							printf("L'ho trovata, e' il numero %i . \n", first_free_source);
							stop = 1;
							break;
						}
					}
					
					if (stop) {
						break;
					}
				    
				}
			} else if (x >= mph && y < mpw){
				/* q3 */
				for(i = SO_HEIGHT-1; i >= 0; i--){
					for(j = 0; j < SO_WIDTH; j++){
						if (pointer_at_map->mappa[i][j].cell_type == 3 && 
							(pointer_at_map->mappa[i][j].active_taxis < pointer_at_map->mappa[i][j].taxi_capacity)) {
							first_free_source = pointer_at_map->mappa[i][j].vertex_number;
							printf("L'ho trovata, e' il numero %i . \n", first_free_source);
							stop = 1;
							break; 
						}
					}
					
					if (stop) {
						break;
					}
				    
				}
			} else {	
				/* q4 (x >= mph && y >= mpw) */
				for(i = SO_HEIGHT-1; i >= 0; i--){
					for(j = SO_WIDTH-1; j >= 0; j--){
						if (pointer_at_map->mappa[i][j].cell_type == 3 && 
							(pointer_at_map->mappa[i][j].active_taxis < pointer_at_map->mappa[i][j].taxi_capacity)) {
							first_free_source = pointer_at_map->mappa[i][j].vertex_number;
							printf("L'ho trovata, e' il numero %i . \n", first_free_source);
							stop = 1;
							break; 
						}
					}
					
					if (stop) {
						break;
					}
				    
				}
			}
			
			/* Cerco un percorso da dove sono alla prima source libera */
			/* printf("Cerco un percorso alla cella SOURCE\n"); */
			printf("Provo a cercare un percorso da una non source \n");
			find_path(pointer_at_map->mappa[x][y].vertex_number, first_free_source);
			printf("Ora provo a muovermi, sono il processo %i \n", getpid()); 
			/* Controllo per eventuali errori DA TOGLIERE */
			for (i = 0; i <= length_of_path; i++){
				if (path_to_follow[i] < 0) printf("SPERO DI NON VEDERLO MAI \n");
			}
			
			printf("Parto alla volta di una source \n");

			move();

			/* Quando finisco qua dovrei essere arrivato */
			/* Verifico di essere arrivato a destinazione */
			if (pointer_at_map->mappa[x][y].vertex_number == path_to_follow[length_of_path]) {
				printf("Sono giunto a destinazione \n");
				/* Libero l'array contente il path */
				free(path_to_follow);
				path_to_follow = NULL; 
			}
		}

	}
	printf("Occhio che esci dal ciclo \n");

#endif

#ifdef DEBUG_STAMPA_MAPPA    
	printf("Uso il metodo di stampa tradizionale \n");
	map_print(pointer_at_map);
#endif

	/* ----------------------- OLTRE QUESTA LINEA SOLO COSE DA CACELLARE --------------------------- */
	/*Libero la matrice dei costi solo alla fine di tutti i viaggi */

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
	/*
	printf(" \n");
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			printf ("%i \t", pointer_at_map->mappa[i][j].vertex_number);
		}
		printf("\n");
	}
	*/
}
