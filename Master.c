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
#include "Map.h"

/****************** Prototipi ******************/
void addEdge(int ** pointer, int i, int j);
void createAdjacencyMatrix();
void kill_all();
void reading_input_values ();
int  max_hole_width();
int  max_hole_height(); 
void random_cell_type(map *pointer_at_map);
void random_taxi_capacity(map *pointer_at_map);
void random_travel_time(map *pointer_at_map);
void map_creation(map *pointer_at_map);
void map_print(map *pointer_at_map);
void map_setup(map *pointer_at_map);
void free_map(map *pointer_at_map);

/* ---------------- Variabili globali ----------------- */
/* SO_WIDTH e SO_HEIGHT sono delle define im map.h */
map mappa;
map *pointer_at_map = &mappa;  
int SO_HOLES = 0;
int SO_TOP_CELLS = 0;
int SO_SOURCES = 0;
int SO_CAP_MIN = 0;
int SO_CAP_MAX = 0;
int SO_TAXI = 0;
int SO_TIMENSEC_MIN = 0;
int SO_TIMENSEC_MAX = 0;
int SO_TIMEOUT = 0;
int SO_DURATION = 0;
int number_of_vertices = 0;
int adjacency_matrix_shm_id;
/* Variabili per la gestione della mappa*/
/* Argomenti da passare alla execve */
char * args_source[] = {"Source", NULL, NULL};
char * args_taxi[] = {"Taxi", NULL, NULL, NULL, NULL};
char * map_shm_id_execve;
char * adjacency_matrix_shm_id_execve;
int map_shm_id; /* valore ritornato da shmget() */
int source_sem_id; /* valore ritornato da semget() per i SOURCE */
int taxi_sem_id;
int * pointer_at_msgq;
int adjacency_matrix_shm_id;
pid_t * child_source;
pid_t * child_taxi;
struct sigaction sa; 
static int taxi_ready = 0;

/* ---------------- Lettura parametri da file ----------------- */
void reading_input_values () {

	char tmpstr1[16];
	char tmpstr2[16];
	char tempbuff[100];
	FILE *input = fopen("Parameters.txt", "r");

	if (input == NULL) {
		printf ("Errore, non riesco ad aprire il file \n");
		exit(EXIT_FAILURE); /* oppure return -1 */
	}

	while(!feof(input)) {

		if (fgets(tempbuff, sizeof(tempbuff),input)) {
			sscanf(tempbuff, "%15s = %15[^;];", tmpstr1, tmpstr2);

			if (strcmp(tmpstr1,"SO_HOLES")==0) {
				SO_HOLES = atoi(tmpstr2);
			}       
			else if (strcmp(tmpstr1,"SO_TOP_CELLS")==0) {
				SO_TOP_CELLS = atoi(tmpstr2);

			}
			else if (strcmp(tmpstr1,"SO_SOURCES")==0) {
				SO_SOURCES = atoi(tmpstr2);
			}
			else if (strcmp(tmpstr1,"SO_CAP_MIN")==0) {
				SO_CAP_MIN = atoi(tmpstr2);
			}  
			else if (strcmp(tmpstr1,"SO_CAP_MAX")==0) {
				SO_CAP_MAX = atoi(tmpstr2);
			}
			else if (strcmp(tmpstr1,"SO_TAXI")==0) {
				SO_TAXI = atoi(tmpstr2);
			}
			else if (strcmp(tmpstr1,"SO_TIMENSEC_MIN")==0) {
				SO_TIMENSEC_MIN = atoi(tmpstr2);
			}
			else if (strcmp(tmpstr1,"SO_TIMENSEC_MAX")==0) {
				SO_TIMENSEC_MAX = atoi(tmpstr2);
			}
			else if (strcmp(tmpstr1,"SO_TIMEOUT")==0) {
				SO_TIMEOUT = atoi(tmpstr2);
			}
			else if (strcmp(tmpstr1,"SO_DURATION")==0) {
				SO_DURATION = atoi(tmpstr2);
			}
			else {
				printf("Parametro non riconosciuto : \"%s\"\n", tmpstr1);
			}
		}
	}
	fclose(input);
	/* Controlli sui parametri */
	if (SO_HOLES < 1 || SO_HOLES > (max_hole_width() * max_hole_height())) {
		printf("Errore, parametro SO_HOLES non valido. Esco.\n");
		exit(EXIT_FAILURE);
	}
	if (SO_TOP_CELLS < 1 || SO_TOP_CELLS > (SO_WIDTH*SO_HEIGHT)-SO_HOLES) {
		printf("Errore, parametro SO_TOP_CELLS non valido. Esco.\n");
		exit(EXIT_FAILURE);
	}
	if (SO_SOURCES < 1 || SO_SOURCES > (SO_WIDTH*SO_HEIGHT)-SO_HOLES) {
		printf("Errore, parametro SO_SOURCES non valido. Esco.\n");
		exit(EXIT_FAILURE);
	}
	if (SO_CAP_MIN < 0) {
		printf("Errore, parametro SO_CAP_MIN non valido. Esco.\n");
		exit(EXIT_FAILURE);
	}
	if (SO_CAP_MAX < SO_CAP_MIN) {
		printf("Errore, parametro SO_CAP_MAX non valido. Esco.\n");
		exit(EXIT_FAILURE);
	}
	if (SO_TAXI < 0 || SO_TAXI > (SO_CAP_MAX * ((SO_WIDTH*SO_HEIGHT)-SO_HOLES))) {
		printf("Errore, parametro SO_TAXI non valido. Esco.\n");
		exit(EXIT_FAILURE);
	}
	if (SO_TIMENSEC_MIN < 0) {
		printf("Errore, parametro SO_TIMENSEC_MIN non valido. Esco.\n");
		exit(EXIT_FAILURE);
	}
	if (SO_TIMENSEC_MAX < SO_TIMENSEC_MIN) {
		printf("Errore, parametro SO_TIMENSEC_MAX non valido. Esco.\n");
		exit(EXIT_FAILURE);
	}
	if (SO_TIMEOUT < 0) {
		printf("Errore, parametro SO_TIMEOUT non valido. Esco.\n");
		exit(EXIT_FAILURE);
	}
	if (SO_DURATION < SO_TIMEOUT) {
		printf("Errore, parametro SO_DURATION non valido. Esco.\n");
		exit(EXIT_FAILURE);
	}

	/* Evito mappe di una sola cella */
	if ((SO_WIDTH == 1) && (SO_HEIGHT == 1)){
		printf("Errore, non posso avere mappe di una sola cella. Esco. \n");
		exit(EXIT_FAILURE);
	}

#ifdef STAMPA_PARAMETRI
	printf("SO_HOLES : %i\n", SO_HOLES);
	printf("SO_TOP_CELLS : %i\n", SO_TOP_CELLS);
	printf("SO_CAP_MIN : %i\n", SO_CAP_MIN);
	printf("SO_CAP_MAX : %i\n", SO_CAP_MAX);
	printf("SO_TAXI : %i\n", SO_TAXI);
	printf("SO_TIMENSEC_MIN : %i\n", SO_TIMENSEC_MIN);
	printf("SO_TIMENSEC_MAX : %i\n", SO_TIMENSEC_MAX);
	printf("SO_TIMEOUT : %i\n", SO_TIMEOUT);
	printf("SO_DURATION : %i\n", SO_DURATION);
	printf("SO_HOLES : %i\n", SO_HOLES);
#endif
	number_of_vertices = (SO_HEIGHT*SO_WIDTH) - SO_HOLES;
}

int max_hole_width() {
	int max_so_width = SO_WIDTH;
	if (SO_WIDTH % 2 != 0){
		max_so_width ++;
		max_so_width = max_so_width / 2;
		return max_so_width;
	} else return max_so_width / 2;
}

int max_hole_height() {
	int max_so_height = SO_HEIGHT;
	if (SO_HEIGHT % 2 != 0){
		max_so_height ++;
		max_so_height = max_so_height / 2;
		return max_so_height;
	} else return max_so_height / 2;
}

/* ---------------- Metodi mappa ----------------- */
#ifdef MAPPA_VALORI_CASUALI
/* Inizializza cell_type in modo casuale */
void random_cell_type(map *pointer_at_map) {
	/* Variabili locali utilizzate:
	 * - value assume il valore della codifica (0 hole, 1 no SO_SOURCES, 2 cella libera) 
	 *   da assegnare alla cella[i][j]; 
	 * - i e j assumono il valore degli indici della cella di riferimento;
	 * - row_pos (row position) e col_pot (column position) sono variabili utilizzate 
	 *   per capire in seguito quale case eseguire all'interno degli switch;
	 * - so_holes e so_sources assumono i valori delle variabili SO_HOLES ed SO_SOURCES,
	 *   dato che quest'ultime sono variabili globali, una scelta implementativa migliore
	 *   e' di non modificarne il valore ma bensi' utilizzarne una copia 
	 *   all'interno della funzione in questione
	 */
	int value, i, j, row_pos, col_pos, so_holes, so_sources;
	so_holes = SO_HOLES;
	so_sources = SO_SOURCES;
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			if (i == 0) { row_pos = 0; } else row_pos = 1;
			if (j == 0) { col_pos = 0; } else if (j > 0 && j < (SO_WIDTH-1)) { col_pos = 1; } else col_pos = 2;
			if (row_pos == 0 && col_pos == 0) {
				do {
					value = rand() % (2-0+1) + 0;
				} while ((value == 0 && so_holes == 0) || (value == 1 && so_sources == 0));
				if (value == 0) so_holes = so_holes-1;
				if (value == 1) so_sources = so_sources-1;
				pointer_at_map->mappa[i][j].cell_type = value;
			} else {    
				switch (row_pos) {
					case 0:
						if (pointer_at_map->mappa[i][j-1].cell_type == 0) { 
							do {
								value = rand() % (2-1+1) + 1;
							} while (value == 1 && so_sources == 0);
							if (value == 1) so_sources = so_sources-1;
							pointer_at_map->mappa[i][j].cell_type = value;
						} else {
							do {
								value = rand() % (2-0+1) + 0;
							} while ((value == 0 && so_holes == 0) || (value == 1 && so_sources == 0));
							if (value == 0) so_holes = so_holes-1;
							if (value == 1) so_sources = so_sources-1;
							pointer_at_map->mappa[i][j].cell_type = value;
						}
						break;
					case 1:
						switch (col_pos) {
							case 0:
								if ((pointer_at_map->mappa[i-1][j].cell_type != 0) && 
										(pointer_at_map->mappa[i-1][j+1].cell_type != 0)) {
									do {
										value = rand() % (2-0+1) + 0;
									} while ((value == 0 && so_holes == 0) || 
											(value == 1 && so_sources == 0));
									if (value == 0) so_holes = so_holes-1;
									if (value == 1) so_sources = so_sources-1;
									pointer_at_map->mappa[i][j].cell_type = value;
								} else {
									do {
										value = rand() % (2-1+1) + 1;
									} while (value == 1 && so_sources == 0);
									if (value == 1) so_sources = so_sources-1;
									pointer_at_map->mappa[i][j].cell_type = value;
								}
								break;
							case 1:
								if ((pointer_at_map->mappa[i][j-1].cell_type != 0) && 
										(pointer_at_map->mappa[i-1][j-1].cell_type != 0) &&
										(pointer_at_map->mappa[i-1][j].cell_type != 0) && 
										(pointer_at_map->mappa[i-1][j+1].cell_type != 0)) {
									do {
										value = rand() % (2-0+1) + 0;
									} while ((value == 0 && so_holes == 0) || 
											(value == 1 && so_sources == 0));
									if (value == 0) so_holes = so_holes-1;
									if (value == 1) so_sources = so_sources-1;
									pointer_at_map->mappa[i][j].cell_type = value;
								} else {
									do {
										value = rand() % (2-1+1) + 1;
									} while (value == 1 && so_sources == 0);
									if (value == 1) so_sources = so_sources-1;
									pointer_at_map->mappa[i][j].cell_type = value;
								}
								break;
							case 2:
								if ((pointer_at_map->mappa[i][j-1].cell_type != 0) && 
										(pointer_at_map->mappa[i-1][j-1].cell_type != 0) &&
										(pointer_at_map->mappa[i-1][j].cell_type != 0)) {
									do {
										value = rand() % (2-0+1) + 0;
									} while ((value == 0 && so_holes == 0) || 
											(value == 1 && so_sources == 0));
									if (value == 0) so_holes = so_holes-1;
									if (value == 1) so_sources = so_sources-1;
									pointer_at_map->mappa[i][j].cell_type = value;
								} else {
									do {
										value = rand() % (2-1+1) + 1;
									} while (value == 1 && so_sources == 0);
									if (value == 1) so_sources = so_sources-1;
									pointer_at_map->mappa[i][j].cell_type = value;
								}
								break;
							default:
								printf("Errore\n");
								exit(EXIT_FAILURE);
						}
						break;
					default:
						printf("Errore\n");
						exit(EXIT_FAILURE);
				}   
			}
		} 
	}
}
#endif
/* Nei casi in cui si odvesse verificare qualche anomalia viene restituito 1, 
 * ma per generare un errore cosa possiamo fare?
 */

/* Assegna ad ogni cella taxi_capacity*/
void random_taxi_capacity(map *pointer_at_map) {
	int i, j;
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			if (pointer_at_map->mappa[i][j].cell_type != 0) { 
				pointer_at_map->mappa[i][j].taxi_capacity = (rand() % (SO_CAP_MAX - SO_CAP_MIN + 1)) + SO_CAP_MIN;
			}
		}
	}
}

/* Assegna ad ogni cella travel_time*/
void random_travel_time(map *pointer_at_map) {
	int i, j;
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			if (pointer_at_map->mappa[i][j].cell_type != 0) { 
				pointer_at_map->mappa[i][j].travel_time = (rand() % (SO_TIMENSEC_MAX - SO_TIMENSEC_MIN + 1)) + SO_TIMENSEC_MIN;
			}
		}
	}
}


/* Da modificare: dovrà leggere i parametri da file e con rand
 * impostare i vari campi della struct. 
 * Se la mappa generata non è corretta si termina con errore
 * La codifica è: 0 hole, 1 SO_SOURCES, 2 no SO_SOURCES
 * Al momento usiamo una mappa 5x4 (quindi non importa quello che date
 * al programma in input che larghezza e altezza, ne faccio override nel
 * main) se volete lo schema della mappa che sto usando lo trovate nel 
 * documento condiviso
 */
void map_setup(map *pointer_at_map) {
	int i, j, max_taxi_map = 0, condizione_ok = 0, counter = 0; /* Counter mi serve per i vertici */
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			/* Imposto ogni cella con cell_type=2, active_taxis=0 */
			pointer_at_map->mappa[i][j].cell_type = 2;
			pointer_at_map->mappa[i][j].active_taxis = 0;
			pointer_at_map->mappa[i][j].crossings = 0;
		}
	}
	do {
		random_taxi_capacity(pointer_at_map);
		for (i = 0; i < SO_HEIGHT; i++) {
			for (j = 0; j < SO_WIDTH; j++) {
				/* Calcolo il numero massimo di taxi sulla mappa con le capienze assegnate */
				max_taxi_map = max_taxi_map + pointer_at_map->mappa[i][j].taxi_capacity;
			}
		}
		/* Controllo che SO TAXI non sia > capacità di taxi massima della mappa */
		if (SO_TAXI > max_taxi_map) {
			printf("Errore: dovrei mettere %i taxi, ma la capeinza massima della mappa e' %i \n", SO_TAXI, max_taxi_map);
			printf("Assegno nuove capienze massime. \n");
		} else condizione_ok = 1;
	} while (condizione_ok == 0); /* Potenziale loop infinito se i parametri sono volutamente sbagliati */
	random_travel_time(pointer_at_map);
	/* Li porto fuori dall'ifdef perché voglio avere */
	/* Per il momento la teniamo ma se ci fa impazzire torna nell'ifdef */
#ifdef MAPPA_VALORI_CASUALI
	random_cell_type(pointer_at_map);
	/* Controlli su mappe particolari */
	/* Mappe di una sola riga */
	if (SO_HEIGHT == 1 && SO_WIDTH > 1) {
		for(j = 1; j < SO_WIDTH - 1; j++){
			if (pointer_at_map->mappa[0][j].cell_type == 0) {
				printf("Errore: non posso avere celle holes in mezzo. Termino.\n");
				exit(EXIT_FAILURE);
			}
		}
	}   
	/* Mappe di una sola colonna */
	if (SO_WIDTH == 1 && SO_HEIGHT > 1) {
		for(i = 1; i < SO_HEIGHT - 1; i++){
			if (pointer_at_map->mappa[i][0].cell_type == 0) {
				printf("Errore: non posso avere celle holes in mezzo. Termino.\n");
				exit(EXIT_FAILURE);
			}
		}
	}
#endif
#ifndef MAPPA_VALORI_CASUALI
	pointer_at_map->mappa[0][0].cell_type = 0;
	pointer_at_map->mappa[1][3].cell_type = 0;
	pointer_at_map->mappa[3][2].cell_type = 0;
	pointer_at_map->mappa[2][2].cell_type = 1;
#endif
	for (i = 0; i < SO_HEIGHT; i++){
		for(j = 0; j < SO_WIDTH; j++){
			if (pointer_at_map->mappa[i][j].cell_type != 0) {
				/* printf("Almeno una volta lo faccio\n"); */
				pointer_at_map->mappa[i][j].vertex_number = counter;
				counter++;
			} else {
				pointer_at_map->mappa[i][j].vertex_number = -1;
			}
		}
	}
	/* https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand */
}

/* Dovrebbe andare */
void map_print(map *pointer_at_map) {
	int i, j;
#ifdef PRINT_MAP_VERTEX_NUMBER    
	int k, l;
#endif    
	pointer_at_map = shmat(map_shm_id, NULL, SHM_FLG); 
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			printf ("%i ", pointer_at_map->mappa[i][j].cell_type);

#ifdef STAMPA_VALORI_CELLA
			printf ("%i", pointer_at_map->mappa[i][j].taxi_capacity);
			printf ("%i", pointer_at_map->mappa[i][j].active_taxis);
			printf ("%i", pointer_at_map->mappa[i][j].travel_time);
			printf ("%i", pointer_at_map->mappa[i][j].crossings);
#endif
#ifdef PRINT_MAP_VERTEX_NUMBER /* Loop infinito da sistemare */
			for (k = 0; k < SO_HEIGHT; i++){
				for (l = 0; l < SO_WIDTH; l++){
					printf("%i \t", pointer_at_map->mappa[k][l].vertex_number);
				}   
				printf("\n");
			}
#endif  
		}
		printf("\n");
	}
	printf("Stampo la matrice di vertici \n");
	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
			printf("%i ", pointer_at_map->mappa[i][j].vertex_number);
		}
		printf("\n");
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


/* Aggiunge gli archi */
void addEdge(int ** pointer, int i, int j) {
	pointer[i][j] = 1;
}

void createAdjacencyMatrix(map *pointer_at_map){

	int i, j;
	int ** pointer;
	int dimension = (number_of_vertices*number_of_vertices)*sizeof(int);

	pointer_at_map = shmat(map_shm_id, NULL, SHM_FLG);

	/* Creo il segmento di memoria condivisa */
	adjacency_matrix_shm_id = shmget(IPC_PRIVATE, dimension, SHM_FLG);
	if (adjacency_matrix_shm_id < 0){
		perror("Non riesco a creare la memoria condivisa. Termino.");
		kill_all();
		exit(EXIT_FAILURE);
	}

	/* Mi attacco come master alla matrice per inizializzarla */
	pointer = shmat(adjacency_matrix_shm_id, NULL, SHM_FLG);
	if (pointer == NULL){
		perror("Non riesco ad attaccarmi alla memoria condivisa con la matrice adiacente. Termino.");
		kill_all();
		exit(EXIT_FAILURE);
	}

	create_index((void*)pointer, number_of_vertices, number_of_vertices, sizeof(int));

	/* La inizializzo a 0 */
	for (i = 0; i < number_of_vertices; i ++){
		for (j = 0; j < number_of_vertices; j ++){
			pointer[i][j] = 0;
		}
	}

	/* Aggiungo tutti gli archi alla matrice adiacente  */
	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
			if (pointer_at_map->mappa[i][j].vertex_number != -1) { 

				/* Aggiungo destra se esiste */
				if (((j+1) < SO_WIDTH) && pointer_at_map->mappa[i][j+1].vertex_number != -1) {
					addEdge(pointer, pointer_at_map->mappa[i][j].vertex_number, pointer_at_map->mappa[i][j+1].vertex_number);
				}
				/* Aggiungo sotto se esiste */
				if (((i+1) < SO_HEIGHT) && pointer_at_map->mappa[i+1][j].vertex_number != -1) {
					addEdge(pointer, pointer_at_map->mappa[i][j].vertex_number, pointer_at_map->mappa[i+1][j].vertex_number);
				}
				/* Aggiungo sinistra se esiste */
				if (((j-1) >= 0) && pointer_at_map->mappa[i][j-1].vertex_number != -1) {
					addEdge(pointer, pointer_at_map->mappa[i][j].vertex_number, pointer_at_map->mappa[i][j-1].vertex_number);
				}
				/* Aggiungo sopra se esiste */
				if (((i-1) >= 0) && pointer_at_map->mappa[i-1][j].vertex_number != -1) {
					addEdge(pointer, pointer_at_map->mappa[i][j].vertex_number, pointer_at_map->mappa[i-1][j].vertex_number);
				}    
			}   
		}
	}


#ifdef PRINT_ADJACENCY_MATRIX   
	/* stampo la matrice */
	printf("Il valore di matrix dim è %i \n", number_of_vertices); 
	printf("La matrica adiacente prima della chiamata è \n");
	for(i = 0; i < number_of_vertices; i++){
		for (j = 0; j < number_of_vertices; j++){
			printf("%i ", pointer[i][j]);    
		}       
		printf("\n");
	}
#endif
	/* Imposto gli zeri ad INFINITY cosi non lo devo fare ogni volta */
	for (i = 0; i < number_of_vertices; i++){
		for (j = 0; j < number_of_vertices; j++){
			if (pointer[i][j] == 0){
				pointer[i][j] = INFINITY;
			}
		}
	}

	/* Salvo l'id della matrice adiacente per i taxi */
	adjacency_matrix_shm_id_execve = malloc(sizeof(int));
    args_taxi[2] = adjacency_matrix_shm_id_execve;
    sprintf(adjacency_matrix_shm_id_execve, "%d", adjacency_matrix_shm_id);
}

void createIPC(map *pointer_at_map) {
	int i, j, counter = 0;
	/* Path per la ftok */
	char *path = "/tmp";
	/* Creo la memoria condivisa che contiene la mappa */
	map_shm_id = shmget (IPC_PRIVATE, sizeof(map), SHM_FLG);
	if (map_shm_id == -1) {
		perror("Non riesco a creare la memoria condivisa. Termino.");
		kill_all();
		exit(EXIT_FAILURE);
	}
	/* Mi attacco come master alla mappa */
	pointer_at_map = shmat(map_shm_id, NULL, SHM_FLG);
	if (pointer_at_map == NULL) {
		perror("Non riesco ad attaccarmi alla memoria condivisa con la mappa. Termino.");
		kill_all();
		exit(EXIT_FAILURE);
	}
	/* Inizializziamo la mappa */
	map_setup(pointer_at_map);
	/* Preparo gli argomenti per la execve */
    map_shm_id_execve = malloc(sizeof(int));
	args_source[1] = args_taxi[1] = map_shm_id_execve;
    sprintf(map_shm_id_execve, "%d", map_shm_id);
    /* Creo il semaforo mutex per l'assegnazione delle celle di SOURCE */
	source_sem_id = semget(SOURCE_SEM_KEY, 1, 0600 | IPC_CREAT);
	if (source_sem_id == -1){
		perror("Non riesco a generare il semaforo per Source. Termino");
		kill_all();
		exit(EXIT_FAILURE);
	} 
	/* Imposto il semaforo con valore 1 -MUTEX */
	if (semctl(source_sem_id, 0, SETVAL, 1) == -1) {
		perror("Non riesco a impostare il semaforo per Source. Termino");
		kill_all();
		exit(EXIT_FAILURE);
	}
	/* Creo l'array di semafori per i TAXI*/
	taxi_sem_id = semget(TAXI_SEM_KEY, TAXI_SEM_ARRAY_DIM, 0600 | IPC_CREAT);
	if (taxi_sem_id == -1){
		perror("Non riesco a generare il semaforo per Taxi. Termino");
		kill_all();
		exit(EXIT_FAILURE);
	}
	/* Assegno il numero del semaforo Taxi di riferimento ad ogni cella */
	counter = 0;
	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
			if (pointer_at_map->mappa[i][j].cell_type != 0) {
				if(semctl(taxi_sem_id, counter, SETVAL, pointer_at_map->mappa[i][j].taxi_capacity) == -1 ){
					perror("Non riesco a impostare il semaforo per Taxi. Termino");
					kill_all();
					exit(EXIT_FAILURE);
				}
				pointer_at_map->mappa[i][j].reference_sem_number = counter;
				counter++;
			}
		}
	}
	/* Creiamo le code di messaggi per le celle source */
	pointer_at_msgq = malloc(SO_SOURCES*sizeof(int));
	for (i = 0; i < SO_SOURCES; i ++) {
		pointer_at_msgq[i] = ftok(path, i);
		if(msgget(pointer_at_msgq[i], 0600 | IPC_CREAT | IPC_EXCL) == 1) {
			perror("Non riesco a creare la coda di messaggi");
			kill_all();
			exit(EXIT_FAILURE);
		}
	}
	/* Assegna al campo della cella il valore della sua coda di messaggi*/
	counter = 0;
	for (i = 0; i < SO_HEIGHT; i ++){
		for (j = 0; j < SO_WIDTH; j++) {
			if (pointer_at_map->mappa[i][j].cell_type == 1) { 
				pointer_at_map->mappa[i][j].message_queue_key = pointer_at_msgq[counter];
				counter++;
			}
		}
	} 

}

void kill_all() {
	/* Completare. Dovrà terminare le risorse IPC che allocheremo. */
	int msqid, i;
	/* Marco per la deallocazione la memoria condivisa */
	shmctl(map_shm_id, IPC_RMID, NULL);
	/* Dealloco il semaforo per Source */
	semctl(source_sem_id, 0, IPC_RMID);
	/* Dealloco l'array di semafori per Taxi */
	semctl(taxi_sem_id, 0, IPC_RMID);
	for (i = 0; i < SO_SOURCES; i++) {
		msqid = msgget(pointer_at_msgq[i], 0600);
		if (msqid < 0) {
			perror("Errore nella deallocazione ");
		}
		/* printf("Sto deallocando la coda di messaggi. Ha id %i \n", msqid); */
		/* Dealloco quella coda di messaggi */
		msgctl(msqid , IPC_RMID , NULL);
		/* msgctl(msqid+1, IPC_RMID, NULL); */
	}
	free(pointer_at_msgq);
	shmctl(adjacency_matrix_shm_id, IPC_RMID, NULL);
#if 0	
	free(info_process_taxi);
	free(info_process_source); 
#endif
}


void taxi_handler(int signum) {
	int j;
	taxi_ready++;
	printf("Mi e' arrivato il segnale numero %i \n", taxi_ready);
	if (taxi_ready == SO_TAXI) {
		for (j = 0; j < SO_TAXI; j++){
			kill(child_taxi[j], SIGCONT);
		}
	}
}


/* Main */
int main () {

	int i, j, valore_fork_sources, valore_fork_taxi;
	
    struct timeval time;
	
    int created_at_start = 0;
	char * creation_moment;
	
    gettimeofday(&time, NULL);
    srand((time.tv_sec * 1000) + (time.tv_usec / 1000)); 
	
	
	bzero(&sa, sizeof(sa));
	sa.sa_handler = taxi_handler;
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);
	

	/* Lettura degli altri parametri specificati da file */
	reading_input_values();
	/* Creo gli oggetti ipc */
	createIPC(pointer_at_map);

	createAdjacencyMatrix(pointer_at_map);
	
	/* Creo l'array dove salvo le dimensione dei figli */
	child_source = calloc(SO_SOURCES, sizeof(pid_t));	
	child_taxi = calloc(SO_TAXI, sizeof(pid_t));

	/* Creo la stringa per informare il taxi su quando e' stato creato */
	creation_moment = malloc(sizeof(int));
	args_taxi[3] = creation_moment;
	sprintf(creation_moment, "%d", created_at_start); 
	
	/* Creo processi SO_SOURCES. Sistema gli argomenti */
	for (i = 0; i < SO_SOURCES; i++) {
		switch(valore_fork_sources = fork()) {
			case -1:
				printf("Errore nella fork. Esco.\n");
				kill_all();
				exit(EXIT_FAILURE);
				break;
			case 0:
				execve("Source", args_source, NULL);
				TEST_ERROR;
				break;
			default:
				/* Codice che voglio esegua il Master */
				/* Magari salviamo le informazioni dei figli dentro 
				   la struct che creiamo all'inizio? */
				child_source[i] = valore_fork_sources;
				break;
		}
	}

	/* Creo processi Taxi. Sistema gli argomenti */
	for (j = 0; j < SO_TAXI; j++) {
		switch(valore_fork_taxi = fork()) {
			case -1:
				printf("Errore nella fork. Esco.\n");
				kill_all();
				exit(EXIT_FAILURE);
				break;
			case 0:
				execve("Taxi", args_taxi, NULL);
				TEST_ERROR;
				break;
			default:
				/* Codice che voglio esegua il Master */
				child_taxi[j] = valore_fork_taxi;
				break;
		}
	}

	/* Aspetto la terminazione dei figli */
	while(wait(NULL) != -1) {
	}
	printf("Stampo tutte le informazioni dei figli e se crasha mi ammazzo \n");
	for (i = 0; i < SO_SOURCES; i++){
		printf("%i  ", child_source[i]);	
	}
	printf("\n");
	for (i = 0; i < SO_TAXI; i++){
		printf("%i  ", child_taxi[i]);
	}
	printf("\n");
#if 0
	printf("Stampo tutte le informazioni dei figli \n");
	for (i = 0; i < SO_SOURCES; i++){
		printf("Stampo pid %i e tipo %d \n", info_process_source[i].child_pid, info_process_source[i].type); 
	}
	for (i = 0; i < SO_SOURCES; i++){
		printf("Stampo pid %i e tipo %d \n", info_process_source[i].child_pid, info_process_source[i].type); 
	}
#endif

	map_print(pointer_at_map);
	printf("IL NUMERO DI TAXI ATTIVI \n");
	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
			printf("%i   ", pointer_at_map->mappa[i][j].active_taxis);
		}
		printf("\n");
	}
	/* Stampo la coda di messaggi della cella 2.2 */
	/* Dealloca la memoria condivisa dove ho la mappa */
	kill_all();
	return 0;
}
