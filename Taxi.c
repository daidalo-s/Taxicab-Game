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
int msg_queue_of_cell_key, msg_queue_of_cell, map_shm_id, taxi_sem_id, adjacency_matrix_shm_id;
int dimension_of_adjacency_matrix;
int start_vertex, destination_vertex;
int element_counter;
int * distance;
int * predecessor;
int * visited;
int * path_to_follow;
int ** pointer_at_adjacency_matrix;
int ** cost;
struct sembuf accesso;
struct sembuf rilascio;
message_queue cell_message_queue;    
map *pointer_at_map; 

/********** PROTOTIPI **********/
void random_cell();
void attach(map *pointer_at_map);
void receive_and_go();
void find_path(int start_vertex, int destination_vertex);
void create_index(void **m, int rows, int cols, size_t sizeElement);
void map_print(map *pointer_at_map);


void random_cell() {
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
	TEST_ERROR
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

void receive_and_go() {
	int x_destination, y_destination;
	printf("Sono il processo Taxi che proverà a ricevere il messaggio \n");
	printf("L'id della coda di messaggi da cui proverò a leggere è %i \n", msg_queue_of_cell_key);
	/* msg_queue_of_cell_key = pointer_at_map->mappa[2][2].message_queue_key; */
	if (msgrcv(msg_queue_of_cell, &cell_message_queue, MESSAGE_WIDTH, 0, 0) < 0) { /* In teoria non ha flag ipcnowait */
		perror("Errore non riesco a ricevere il messaggio\n ");
	}
	x_destination = atoi(&cell_message_queue.message[0]);
	y_destination = atoi(&cell_message_queue.message[2]);
	printf("La x_destination e' % i \n", x_destination);
	printf("La y_destination e' %i \n", y_destination);
	printf("Ho ricevuto il messaggio %s \n", cell_message_queue.message);
	/* Devo chiamare dijkstra: ho bisogno di passare il vertice in cui mi trovo e il vertice in cui voglio andare */
	start_vertex = pointer_at_map->mappa[x][y].vertex_number; /* La cella in cui sono */
	destination_vertex = pointer_at_map->mappa[x_destination][y_destination].vertex_number; /* La cella in cui voglio andare*/
	find_path(start_vertex, destination_vertex);
	/* Accedendo a path to follow il primo elemento e la prossima cella in cui andare */

}


void find_path(int start_vertex, int destination_vertex) {

	int count, min_distance, next_node, i, j;
	int tmp_int;

	printf("La grandezza e %i \n", dimension_of_adjacency_matrix);
	printf("Parto dal vertice numero %i \n", start_vertex);
	printf("Arrivo al vertice numero %i \n", destination_vertex);
#if 1	
	/* Creo la matrice dei costi solo la prima volta*/
	if (cost == NULL){ 
		cost = (int **)malloc(dimension_of_adjacency_matrix * sizeof(int*));
		for (i = 0; i < dimension_of_adjacency_matrix; i++){
			cost[i] = (int *)malloc(dimension_of_adjacency_matrix * sizeof(int));
		}
	}	
#endif
	/* Creo l'array per contenere la distanza, lo faccio ad ogni nuovo viaggio */
	distance = malloc(dimension_of_adjacency_matrix * sizeof(int));
	/* Creo l'array per salvare la provenienza, lo faccio ad ogni nuovo viaggio */
	predecessor = malloc(dimension_of_adjacency_matrix * sizeof(int));
	/* Creo l'array per salvare i vertici visitati, lo faccio ad ogni nuovo viaggio */
	visited = malloc(dimension_of_adjacency_matrix * sizeof(int));

#if 1
	/* Inizializzo la matrice dei costi */
	for (i = 0; i < dimension_of_adjacency_matrix; i ++){
		for (j = 0; j < dimension_of_adjacency_matrix; j++){
			if (pointer_at_adjacency_matrix[i][j] == 0) cost[i][j] = INFINITY;
			else cost[i][j] = pointer_at_adjacency_matrix[i][j];
		}
	}
#endif    
	/*
	   printf("STAMPO LA MATRICE DEI COSTI CREATA \n");
	   for (i = 0; i < dimension_of_adjacency_matrix; i++){
	   for (j = 0; j < dimension_of_adjacency_matrix; j++){
	   printf("%i ", cost[i][j]);
	   }
	   printf("\n");
	   }
	   */

	/* Inizializzo i tre array */
	for (i = 0; i < dimension_of_adjacency_matrix; i++){
		distance[i] = cost[start_vertex][i];
		predecessor[i] = start_vertex;
		visited[i] = 0;
	}

	distance[start_vertex] = 0;
	visited[start_vertex] = 1;
	count = 1;

	while(count < dimension_of_adjacency_matrix-1){
		min_distance = INFINITY;

		/* Trovo il nodo alla distanza minima */
		for (i = 0; i < dimension_of_adjacency_matrix; i++) { 
			if (distance[i] < min_distance && !visited[i]) {
				min_distance = distance[i];
				next_node = i;
			}
		}

		/* Continuo l'esplorazione alla ricerca di un path migliore */
		visited[next_node] = 1;

		for (i = 0; i < dimension_of_adjacency_matrix; i++) {  
			if (!visited[i]) 

				if (min_distance + cost[next_node][i] < distance[i]){
					distance[i] = min_distance + cost[next_node][i];
					predecessor[i] = next_node;
				}	
		}			
		count++;
	} 

	/* Creo l'array dove salvo il percorso che devo seguire */
	tmp_int = distance[destination_vertex] - 1;
	path_to_follow = malloc(tmp_int * sizeof(int));
	element_counter = 0;
	do {
		path_to_follow[tmp_int] = destination_vertex;
		destination_vertex = predecessor[destination_vertex];
		tmp_int --;
		element_counter++;
		printf("Eseguo una volta\n");
	} while (destination_vertex != start_vertex && tmp_int >= 0);
	printf("TEST METODO DIJKSTRA: STAMPO IL PATH PIU BREVE \n");
	for (j = 0; j < element_counter; j++){
		printf("%i \t", path_to_follow[j]);
	}
	printf("\n");
	/* Devo spostare l'array path, non devo pulirlo fino a quando non sono arrivato a destinazione */
	free(distance);
	free(predecessor);
	free(visited);
}

void create_index(void **m, int rows, int cols, size_t sizeElement){
	int i;  
	size_t sizeRow = cols * sizeElement;
	m[0] = m+rows;
	for(i=1; i<rows; i++){      
		m[i] = ((u_int8_t*)m[i-1]+sizeRow);
	}
}

/********** MAIN **********/
/*
 *	All'interno della funzione main il processo deve prendere controllo del segmento di memoria
 *	condivisa dove ho la mappa e dei vari semafori che userà per i suoi metodi.
 */
int main(int argc, char *argv[])
{	
	int i,j,SO_HOLES=0;
	srand(time(NULL));
	/* Prendo l'id e mi attacco al segmento */ 
	map_shm_id = atoi(argv[1]);
	adjacency_matrix_shm_id = atoi(argv[2]);
	pointer_at_map = shmat(map_shm_id, NULL, 0);
	if (pointer_at_map == NULL){
		perror("Processo Taxi: non riesco ad accedere alla mappa. Termino.");
		exit(EXIT_FAILURE);
	}

	pointer_at_adjacency_matrix = (int **)shmat(adjacency_matrix_shm_id, NULL, 0);
	if (pointer_at_adjacency_matrix == NULL){
		perror("Processo Taxi: non riesco ad accedere alla matrice adiacente. Termino.");
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
	attach(pointer_at_map);
	dimension_of_adjacency_matrix = (SO_WIDTH*SO_HEIGHT)-SO_HOLES;
	create_index((void*)pointer_at_adjacency_matrix, dimension_of_adjacency_matrix, dimension_of_adjacency_matrix, sizeof(int));
	printf("Dimensione calcolata %i \n", dimension_of_adjacency_matrix);
	/* printf("Provo a stampare il campo 00 %d \n", pointer_at_adjacency_matrix[0][0]); */
	printf("SONO UN PROCESSO TAXI: STAMPO LA MATRICE ADIACENTE \n");
	for (i = 0; i < dimension_of_adjacency_matrix; i++){
		for (j = 0; j < dimension_of_adjacency_matrix; j++){
			printf("%d ", pointer_at_adjacency_matrix[i][j]); 
		}
		printf("\n");
	}

	receive_and_go();

	printf("TEST METODO DIJKSTRA: STAMPO IL PATH PIU BREVE DAL MAIN\n");
	for (j = 0; j < element_counter; j++){
		printf("%i \t", path_to_follow[j]);
	}
	printf("\n");
	printf("Sono il processo taxi: mi sono attaccato alla cella %i %i \n", x, y);
	printf("Sono il processo taxi: il semaforo ha id %i ed e' il numero %i \n", taxi_sem_id, pointer_at_map->mappa[tmpx][tmpy].reference_sem_number);


	printf("Sono un processo Taxi \n");
	printf("Il campo della cella 2.2 e': %i \n", pointer_at_map->mappa[2][2].cell_type);


#ifdef DEBUG_STAMPA_MAPPA    
	printf("Uso il metodo di stampa tradizionale \n");
	map_print(pointer_at_map);
#endif

	printf("Ora perdo un po' di tempo e poi esco \n");
	sleep(3);
	printf("ho finito di dormire, sono un processo Taxi\n");


	/* ----------------------- OLTRE QUESTA LINEA SOLO COSE DA CACELLARE --------------------------- */
	/*Libero la matrice dei costi solo alla fine di tutti i viaggi */
	for (i = 0; i < dimension_of_adjacency_matrix; i++){
		free(cost[i]);
	}
	free(cost);

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
