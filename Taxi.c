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
int previous_x = 0, previous_y = 0;
int msg_queue_of_cell_key, msg_queue_of_cell, map_shm_id, taxi_sem_id, adjacency_matrix_shm_id;
int num_of_vertices;
int element_counter;
int length_of_path;
int start_sem_id;
int SO_TIMEOUT, SO_TAXI, SO_HOLES;
int ongoing_trip = 0;
int info_taxi_id, info_taxi_sem_id;
int local_longest_trip = 0, local_served_clients = 0, local_service_time = 0, prev_service_time = 0;
int source = 0;
int * distance, * predecessor, * visited, * path_to_follow;
int ** pointer_at_adjacency_matrix;
taxi_info * pointer_at_taxi_info;
struct sembuf accesso, rilascio;
struct sembuf start = {0, -1, 0};
struct sembuf end = {0, +1, 0};
struct sembuf write_info = {0, -1, 0}; 
struct sembuf release_info = {0, +1, 0};
message_queue cell_message_queue;    
map *pointer_at_map; 

/********** PROTOTIPI **********/
void random_cell();
void attach();
void receive_and_find_path();
void find_path(int start_vertex, int destination_vertex);
void create_index(void **m, int rows, int cols, size_t sizeElement);
void taxi_handler (int signum);
void setting_sem_struct (int x, int y);
void addEdge(int ** pointer_at_adjacency_matrix, int i, int j);
void move();
void create_adjacency_matrix();
void kill_all();

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

/* Aggiunge gli archi */
void addEdge(int ** pointer_at_adjacency_matrix, int i, int j) {
	pointer_at_adjacency_matrix[i][j] = 1;
}

void random_cell() {
	
	/* Tiro a caso valori finche' trovo una cella non hole*/
	do {
		random_coordinates[0] = rand() % ((SO_HEIGHT-1) - 0 + 1) + 0; /* x == i */
		random_coordinates[1] = rand() % ((SO_WIDTH-1) - 0 + 1) + 0; /* y == j*/
	} while (pointer_at_map->mappa[random_coordinates[0]][random_coordinates[1]].cell_type == 0);
	
	tmpx = random_coordinates[0]; /* == i */
	tmpy = random_coordinates[1]; /* == j */
	
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

	random_cell();
	/* Entro in sezione critica */
	while (semop(taxi_sem_id, &accesso, 1) == -1) { 
		random_cell();
	}

	x = tmpx;
	y = tmpy;
	pointer_at_map->mappa[x][y].active_taxis++;
	

}  

void receive_and_find_path() {
	
	int x_destination = 0, y_destination = 0;

	int start_vertex = 0, destination_vertex = 0;
	
	while (msgrcv(msg_queue_of_cell, &cell_message_queue, MESSAGE_WIDTH, 1, 0) < 0){
	
	}

	x_destination = atoi(&cell_message_queue.message[0]);
	y_destination = atoi(&cell_message_queue.message[2]);
	
	/* Devo chiamare dijkstra: ho bisogno di passare il vertice in cui mi trovo e il vertice in cui voglio andare */
	start_vertex = pointer_at_map->mappa[x][y].vertex_number; /* La cella in cui sono */
	destination_vertex = pointer_at_map->mappa[x_destination][y_destination].vertex_number; /* La cella in cui voglio andare*/
	find_path(start_vertex, destination_vertex);
	/* Accedendo a path to follow il primo elemento è la prossima cella in cui andare */
}


void find_path(int start_vertex, int destination_vertex) {

	int count = 0, min_distance = 0, next_node = 0, i = 0,  tmp_int = 0;
	
	/* Creo l'array per contenere la distanza, lo faccio ad ogni nuovo viaggio */
	distance = (int*)malloc(num_of_vertices * sizeof(int));
	
	/* Creo l'array per salvare la provenienza, lo faccio ad ogni nuovo viaggio */
	predecessor = (int*)malloc(num_of_vertices * sizeof(int));
	
	/* Creo l'array per salvare i vertici visitati, lo faccio ad ogni nuovo viaggio */
	visited = (int*)malloc(num_of_vertices * sizeof(int));
	
	/* Inizializzo i tre array */
	for (i = 0; i < num_of_vertices; i++){
		distance[i] = pointer_at_adjacency_matrix[start_vertex][i];
		predecessor[i] = start_vertex;
		visited[i] = 0;
	}

	distance[start_vertex] = 0;
	visited[start_vertex] = 1;
	count = 1;

	while(count < num_of_vertices-1){
		
		min_distance = INFINITY;
		
		/* Trovo il nodo alla distanza minima */
		for (i = 0; i < num_of_vertices; i++) { 
			if (distance[i] < min_distance && !visited[i]) {
				min_distance = distance[i];
				next_node = i;
			}
		}
		
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

	/* Creo l'array dove salvo il percorso che devo seguire */
	tmp_int = distance[destination_vertex] - 1;
	length_of_path = tmp_int;
	path_to_follow = malloc((tmp_int+1) * sizeof(int));
	element_counter = 0;

	do {
		path_to_follow[tmp_int] = destination_vertex;
		destination_vertex = predecessor[destination_vertex];
		tmp_int --;
		element_counter++;
	} while (destination_vertex != start_vertex);

	printf("\n");
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
	
	sigset_t my_mask;
	sigemptyset(&my_mask);
	sigaddset(&my_mask, SIGALRM);

	prev_service_time = local_service_time;
	/* Finche' sono all'interno dell'array del percorso */
	while (k <= length_of_path) {
		
		stop = 0;

		/* Prendo il tempo della cella in cui mi trovo */
		ts.tv_sec = 0;
		ts.tv_nsec = pointer_at_map->mappa[x][y].travel_time;
		if (source) { 
			local_service_time = local_service_time + (pointer_at_map->mappa[x][y].travel_time / 1000);
		}
		/* Dormo il tempo giusto */
		nanosleep(&ts, NULL);

		pointer_at_map->mappa[x][y].crossings++;
		
		/* Prendo il prossimo vertice */
		next_vertex = path_to_follow[k];
		
		/* Alarm per la terminazione, devo salvare la cella in cui mi trovavo */
		previous_x = x;
		previous_y = y;
 
		/* Aggiorno i valori di x e y: MIGLIORABILE DIVIDENDO LA MAPPA IN QUADRANTI */
		for (i = 0; i < SO_HEIGHT; i++){
			for (j = 0; j < SO_WIDTH; j++){
				if (pointer_at_map->mappa[i][j].vertex_number == next_vertex) {
					/* Esco dalla cella in cui mi trovavo */
					/* Blocco alarm */
					sigprocmask(SIG_BLOCK, &my_mask, NULL);
					pointer_at_map->mappa[x][y].active_taxis = pointer_at_map->mappa[x][y].active_taxis - 1;
					semop(taxi_sem_id, &rilascio, 1);

					setting_sem_struct(i, j);

					semop(taxi_sem_id, &accesso, 1);

					x = i;
					y = j;
			
					pointer_at_map->mappa[x][y].active_taxis++;
					local_longest_trip++;
					/* Sblocco alarm*/
					sigaddset(&my_mask, SIGALRM);
					sigprocmask(SIG_UNBLOCK, &my_mask, NULL);
					stop = 1;
					break;
				}
			}
			if (stop) {
				break;
			}
		}
		k++;
	}

}

void create_adjacency_matrix() {
	
	int i, j;

	/* Calcolo il numero di holes per calcolare la dimensione della matrice adiacente */
	for (i = 0; i < SO_HEIGHT; i ++) {
		for (j = 0; j < SO_WIDTH; j++){
			if (pointer_at_map->mappa[i][j].cell_type == 0) SO_HOLES++;
		}
	}

	num_of_vertices = ((SO_WIDTH*SO_HEIGHT)-SO_HOLES);
	pointer_at_adjacency_matrix = malloc(num_of_vertices*sizeof(int*));
	for (i = 0; i < num_of_vertices; i++){
		pointer_at_adjacency_matrix[i] = malloc(num_of_vertices*sizeof(int));
	}
	
	/* Inizializzo la matrice */
	for (i = 0; i < num_of_vertices; i++){
		for (j = 0; j < num_of_vertices; j++){
			pointer_at_adjacency_matrix[i][j] = 0;
		}
	}
	
	/* Aggiungo gli archi */
	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
			if (pointer_at_map->mappa[i][j].vertex_number != -1) { 

				/* Aggiungo destra se esiste */
				if (((j+1) < SO_WIDTH) && pointer_at_map->mappa[i][j+1].vertex_number != -1) {
					addEdge(pointer_at_adjacency_matrix, pointer_at_map->mappa[i][j].vertex_number, pointer_at_map->mappa[i][j+1].vertex_number);
				}
				/* Aggiungo sotto se esiste */
				if (((i+1) < SO_HEIGHT) && pointer_at_map->mappa[i+1][j].vertex_number != -1) {
					addEdge(pointer_at_adjacency_matrix, pointer_at_map->mappa[i][j].vertex_number, pointer_at_map->mappa[i+1][j].vertex_number);
				}
				/* Aggiungo sinistra se esiste */
				if (((j-1) >= 0) && pointer_at_map->mappa[i][j-1].vertex_number != -1) {
					addEdge(pointer_at_adjacency_matrix, pointer_at_map->mappa[i][j].vertex_number, pointer_at_map->mappa[i][j-1].vertex_number);
				}
				/* Aggiungo sopra se esiste */
				if (((i-1) >= 0) && pointer_at_map->mappa[i-1][j].vertex_number != -1) {
					addEdge(pointer_at_adjacency_matrix, pointer_at_map->mappa[i][j].vertex_number, pointer_at_map->mappa[i-1][j].vertex_number);
				}    
			}   
		}
	}

	/* Imposto gli zeri ad INFINITY cosi non lo devo fare ogni volta nei processi TAXI */
	for (i = 0; i < num_of_vertices; i++){
		for (j = 0; j < num_of_vertices; j++){
			if (pointer_at_adjacency_matrix[i][j] == 0){
				pointer_at_adjacency_matrix[i][j] = INFINITY;
			}
		}
	}

}

void kill_all() {

	int i;

	/* Array */
	if (distance != NULL) free (distance);
	if (predecessor != NULL) free (predecessor);
	if (visited != NULL) free (visited);
	if (path_to_follow != NULL) free (path_to_follow);
	for (i = 0; i < num_of_vertices; i++){
		free (pointer_at_adjacency_matrix[i]);
	}
	free(pointer_at_adjacency_matrix);

	/* Memoria condivisa */
	if (map_shm_id != 0) shmdt(pointer_at_map);
	if (adjacency_matrix_shm_id != 0) shmdt(pointer_at_adjacency_matrix);
	if (info_taxi_id != 0) shmdt(pointer_at_taxi_info);
	
}

void taxi_handler (int signum) {
	
	int i;
	/* Handler dopo SO_TIMEOUT */
	if (signum == SIGTERM) { 
		/* Semop, controllo prima cella vuota, immissione, rilascio */
		semop(info_taxi_sem_id, &write_info, 1);
	
		for(i = 0; i < SO_TAXI; i++){
			if (pointer_at_taxi_info[i].pid == 0) {
				pointer_at_taxi_info[i].pid = getpid();
				pointer_at_taxi_info[i].service_time = local_service_time;
				pointer_at_taxi_info[i].served_clients = local_served_clients;
				pointer_at_taxi_info[i].longest_trip = local_longest_trip;
				break;
			}
		}

		semop(info_taxi_sem_id, &release_info, 1);

		kill_all(); 
		kill(getpid(), SIGKILL);
	}
	
	/* Handler dopo ctrl c*/
	if (signum == SIGINT) {
		kill_all();
		kill(getpid(), SIGKILL);
	}

	/* Alarm SO_TIMEOUT */
	if (signum == SIGALRM) {
		if (previous_x == x && previous_y == y) {
			if (ongoing_trip) {	
				pointer_at_map->mappa[x][y].aborted_trip++;
			}
			pointer_at_map->mappa[x][y].active_taxis--;
			semop(taxi_sem_id, &rilascio, 1);
			semop(start_sem_id, &end, 1);
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
	
	int i,j;

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

	/* Prendo l'id della mappa e mi attacco al segmento */ 
	map_shm_id = atoi(argv[1]);
	pointer_at_map = shmat(map_shm_id, NULL, 0);

	if (pointer_at_map == NULL){
		perror("Processo Taxi: non riesco ad accedere alla mappa. Termino.");
		exit(EXIT_FAILURE);
	}

	create_adjacency_matrix();

	/* Leggo SO_TIMEOUT */
	SO_TIMEOUT = atoi(argv[3]);
	
	/* Leggo SO_TAXI */
	SO_TAXI = atoi(argv[2]);

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

	/* Prendere visibilita' del semaforo mutex per la scrittura finale */
	info_taxi_sem_id = semget(INFO_SEM_KEY, 1, 0600);
	if (info_taxi_sem_id == -1){
		perror("Processo Taxi: non riesco a prendere visibilità de semaforo per la scrittura statistiche. Termino. ");
		exit(EXIT_FAILURE);
	}

	/* Prendere visibilita' della memoria condivisa dove salvare le statistiche */
	info_taxi_id = atoi(argv[4]);
	pointer_at_taxi_info = shmat(info_taxi_id, NULL, 0);
	if (pointer_at_taxi_info == NULL){
		perror("Processo Taxi: non riesco ad accedere alla memoria condivisa per le statistiche. Termino.");
		exit(EXIT_FAILURE);
	}

	/* Ci posizionamo a caso sulla mappa */
	attach();
	
	semop(start_sem_id, &start, 1);
	
	alarm(SO_TIMEOUT);

	while (1) {

		source = stop = ongoing_trip = 0;
			
		/* Devo capire in quale tipologia di cella mi trovo */
		if (pointer_at_map->mappa[x][y].cell_type == 3) {
			source = 1;
		}	
		
		/* Biforcazione in base a dove sono */
		if (source == 1) {

			/* Prende la key per la coda di messaggi */
			msg_queue_of_cell_key = pointer_at_map->mappa[x][y].message_queue_key;		
			
			/* Ci attacchiamo alla coda di messaggi */
			msg_queue_of_cell = msgget(msg_queue_of_cell_key, 0);
			if (msg_queue_of_cell == -1){
				perror("Sono un processo Taxi: non riesco ad attaccarmi alla coda di messaggi della mia cella.");
			}
			
			/* Leggo un messaggio e calcolo il percorso */
			receive_and_find_path();

			ongoing_trip = 1;
			
			/* Parto */
			move();

			/* Verifico di essere arrivato a destinazione */
			if (pointer_at_map->mappa[x][y].vertex_number == path_to_follow[length_of_path]) {
				ongoing_trip = 0;
				pointer_at_map->mappa[x][y].completed_trip++;
				local_served_clients++;
				if (prev_service_time > local_service_time){
					local_service_time = prev_service_time;
				}
				/* Libero l'array contente il path */
				free(path_to_follow);
				path_to_follow = NULL;  

			} else {
				/* Qualcosa e' andato storto nel viaggio, termino */
				pointer_at_map->mappa[x][y].active_taxis--;
				kill(getppid(), SIGUSR2);
				pause();
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
			find_path(pointer_at_map->mappa[x][y].vertex_number, first_free_source);
			
			move();

			/* Verifico di essere arrivato a destinazione */
			if (pointer_at_map->mappa[x][y].vertex_number == path_to_follow[length_of_path]) {
				/* Libero l'array contente il path */
				free(path_to_follow);
				path_to_follow = NULL; 
			}
		}
	}
	return 0;
}
