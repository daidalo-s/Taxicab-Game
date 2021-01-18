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
void reading_input_values ();
int  max_hole_width();
int  max_hole_height();
void random_cell_type();
void random_taxi_capacity();
void random_travel_time(); 
void map_setup();
void check_map();
void map_print();
void addEdge(int ** pointer, int i, int j);
void createAdjacencyMatrix();
void createIPC();
void kill_all();
void taxi_handler(int signum);

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
/* Variabili per la gestione della mappa*/
/* Argomenti da passare alla execve */
int map_shm_id;    /* valore ritornato da shmget(), id del segmento */
int adjacency_matrix_shm_id; /* valore ritornato da shmget, id del segmento */
int source_sem_id; /* valore ritornato da semget() per i SOURCE, id dell'array */
int taxi_sem_id;   /* valore ritornato da semget() per i TAXI, id dell'array*/
int * pointer_at_msgq; /* Malloc di interi dove salviamo le key delle code di messaggi che poi inseriamo nelle celle */
char * map_shm_id_execve; /* Puntatore a char dove salvo l'id della mappa per passarlo ai figli*/
char * adjacency_matrix_shm_id_execve; /* Puntatore a char dove salvo l'id della matrice adiacente per passarlo ai figli*/
char * creation_moment; /* Puntatore a char dove salvo il momento di creazione del taxi, 0 in inizializzazione, 1 dopo timeout*/
char * args_source[] = {"Source", NULL, NULL}; /* Array di argomenti da passare a Source, [1]=id_mappa*/
/* Array di argomenti da passare a Taxi [1]=id_mappa,[2]=matrice_adiacente,[3]=momento creazione*/ 
char * args_taxi[] = {"Taxi", NULL, NULL, NULL, NULL}; 
pid_t * child_source; /* Malloc dove salviamo pid dei figli Source */
pid_t * child_taxi; /* Malloc dove salviamo pid dei figli Taxi */
#if 0
static int taxi_ready = 0; /* Intero utilizzato per l'handler dei segnali mandati dai Taxi*/ 
#endif

/* ---------------- Lettura parametri da file ----------------- */
void reading_input_values () {

	char tmpstr1[16];
	char tmpstr2[16];
	char tempbuff[100];
	FILE *input = fopen("Parameters.txt", "r");

	if (input == NULL) {
		printf ("Errore, non riesco ad aprire il file \n");
		exit(EXIT_FAILURE); 
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
	TEST_ERROR;

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
	/* Lo calcolo qua per poter creare la matrice adiancente della giusta dimensione */
	number_of_vertices = (SO_HEIGHT*SO_WIDTH) - SO_HOLES;
}

/* Metodo utilizzato per capire il massimo numero di Hole*/
int max_hole_width() {
	int max_so_width = SO_WIDTH;
	if (SO_WIDTH % 2 != 0){
		max_so_width ++;
		max_so_width = max_so_width / 2;
		return max_so_width;
	} else return max_so_width / 2;
}

/* Metodo utilizzato per capire il massimo numero di Hole*/
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
void random_cell_type() {

    int num_hole_placed = 0;
    int position;
    int num_source_placed = 0;
    int x, y;
    int i, j;

    /* Inizializzo tutta la mappa a 2 */
    for (i = 0; i < SO_HEIGHT; i++){
    	for (j = 0; j < SO_WIDTH; j++){
    		pointer_at_map->mappa[i][j].cell_type = 2;
    	}
    }

    /* prima posiziono gli hole */
    while (num_hole_placed != SO_HOLES) {
        
        x = rand() % ((SO_HEIGHT-1) - 0 + 1) + 0;
        y = rand() % ((SO_WIDTH-1) - 0 + 1) + 0;

        /* devo capire dove mi trovo */
        if ((x == 0 && y == 0)||(x == 0 && y == SO_WIDTH-1)||(x == SO_HEIGHT-1 && y == 0)||(x == SO_HEIGHT-1 && y == SO_WIDTH -1)){
            if ((x == 0 && y == 0)) {
                position = 1; /* angolo in alto a sinistra */
            } else if ((x == 0 && y == SO_WIDTH-1)){
                position = 2; /* angolo in alto a destra */
            } else if ((x == SO_HEIGHT-1 && y == 0)) {
                position = 3; /* angolo in basso a sinistra */
            } else {
                position = 4; /* angolo in basso a destra */
            }
        } else if ((x == 0) || (y == 0) || (x == SO_HEIGHT -1) || (y == SO_WIDTH-1)){
            if (x == 0){
                position = 5; /* bordo in alto */
            } else if (y == 0) {
                position = 6; /* bordo a sinistra */
            } else if (x == SO_HEIGHT - 1) {
                position = 7; /* bordo sotto */
            } else {
                position = 8; /* bordo a destra */
            }
        } else {
            position = 9; /* mezzo */
        }

        /* devo controllare le celle adiacenti in base al valore di position */
        switch (position)
        {
        case 1: /* angolo in alto a sinistra */
             if ((pointer_at_map->mappa[x][y+1].cell_type != 0 && pointer_at_map->mappa[x+1][y+1].cell_type != 0 && pointer_at_map->mappa[x+1][y].cell_type != 0) &&
                (pointer_at_map->mappa[x][y].cell_type != 0)) {
                pointer_at_map->mappa[x][y].cell_type = 0;
                num_hole_placed++;
            }
            break;

        case 2: /* angolo in alto a destra */
             if ((pointer_at_map->mappa[x][y-1].cell_type != 0 && pointer_at_map->mappa[x+1][y-1].cell_type != 0 && pointer_at_map->mappa[x+1][y].cell_type != 0) &&
                (pointer_at_map->mappa[x][y].cell_type != 0)){
                pointer_at_map->mappa[x][y].cell_type = 0;
                num_hole_placed++;
            }
            break;

        case 3: /* angolo in basso a sinistra */
            if ((pointer_at_map->mappa[x-1][y].cell_type != 0 && pointer_at_map->mappa[x-1][y+1].cell_type != 0 && pointer_at_map->mappa[x][y+1].cell_type != 0) &&
                (pointer_at_map->mappa[x][y].cell_type != 0)){
                pointer_at_map->mappa[x][y].cell_type = 0;
                num_hole_placed++;
            }
            break;
        
        case 4: /* angolo in basso a destra */
            if ((pointer_at_map->mappa[x-1][y].cell_type != 0 && pointer_at_map->mappa[x-1][y-1].cell_type != 0 && pointer_at_map->mappa[x][y-1].cell_type != 0) &&
                (pointer_at_map->mappa[x][y].cell_type != 0)) {
                pointer_at_map->mappa[x][y].cell_type = 0;
                num_hole_placed++;
            }
            break;

        case 5: /* bordo in alto */
             if ((pointer_at_map->mappa[x][y-1].cell_type != 0 && pointer_at_map->mappa[x+1][y-1].cell_type != 0 && pointer_at_map->mappa[x+1][y].cell_type != 0 
                && pointer_at_map->mappa[x+1][y+1].cell_type != 0 && pointer_at_map->mappa[x][y+1].cell_type != 0) && (pointer_at_map->mappa[x][y].cell_type != 0)) {
                    pointer_at_map->mappa[x][y].cell_type = 0;
                    num_hole_placed++;
            }
            break;
        
        case 6: /* bordo a sinistra */
            if (pointer_at_map->mappa[x-1][y].cell_type != 0 && pointer_at_map->mappa[x-1][y+1].cell_type != 0 && pointer_at_map->mappa[x][y+1].cell_type != 0 
                && pointer_at_map->mappa[x+1][y+1].cell_type != 0 && pointer_at_map->mappa[x+1][y].cell_type != 0) {
                    pointer_at_map->mappa[x][y].cell_type = 0;
                    num_hole_placed++;
            }
            break;
        
        case 7: /* bordo sotto */
           if ((pointer_at_map->mappa[x][y-1].cell_type != 0 && pointer_at_map->mappa[x-1][y-1].cell_type != 0 && pointer_at_map->mappa[x-1][y].cell_type != 0 
                && pointer_at_map->mappa[x-1][y+1].cell_type != 0 && pointer_at_map->mappa[x][y+1].cell_type != 0) && (pointer_at_map->mappa[x][y].cell_type != 0)) {
                    pointer_at_map->mappa[x][y].cell_type = 0;
                    num_hole_placed++;
            }
            break;
        
        case 8: /* bordo a destra */
           if ((pointer_at_map->mappa[x-1][y].cell_type != 0 && pointer_at_map->mappa[x-1][y-1].cell_type != 0 && pointer_at_map->mappa[x][y-1].cell_type != 0
                && pointer_at_map->mappa[x+1][y-1].cell_type != 0 && pointer_at_map->mappa[x+1][y].cell_type != 0) && (pointer_at_map->mappa[x][y].cell_type != 0)) { 
                    pointer_at_map->mappa[x][y].cell_type = 0;
                    num_hole_placed++;       
                }
            break;
        
        case 9: /* mezzo */
             if ((pointer_at_map->mappa[x-1][y-1].cell_type != 0 && pointer_at_map->mappa[x-1][y].cell_type != 0 && pointer_at_map->mappa[x-1][y+1].cell_type != 0
                && pointer_at_map->mappa[x][y+1].cell_type != 0 && pointer_at_map->mappa[x+1][y+1].cell_type != 0 && pointer_at_map->mappa[x+1][y].cell_type != 0 
                && pointer_at_map->mappa[x+1][y-1].cell_type != 0 && pointer_at_map->mappa[x][y-1].cell_type != 0) && (pointer_at_map->mappa[x][y].cell_type != 0)) {
                    pointer_at_map->mappa[x][y].cell_type = 0;
                    num_hole_placed++;
                }
            break;
        
        default:
            break;
        }
    }
    /* ora posiziono le source */
    while (num_source_placed != SO_SOURCES)
    {
        x = rand() % ((SO_HEIGHT-1) - 0 + 1) + 0;
        y = rand() % ((SO_WIDTH-1) - 0 + 1) + 0;
        if (pointer_at_map->mappa[x][y].cell_type != 0 && pointer_at_map->mappa[x][y].cell_type != 1){
            pointer_at_map->mappa[x][y].cell_type = 1;
            num_source_placed++;
        }
	}
	printf("Ho posizionato %i source \n", num_source_placed);
}
#endif

void random_taxi_capacity() {
	
	int i, j;
	
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			if (pointer_at_map->mappa[i][j].cell_type != 0) { 
				pointer_at_map->mappa[i][j].taxi_capacity = (rand() % (SO_CAP_MAX - SO_CAP_MIN + 1)) + SO_CAP_MIN;
			}
		}
	}
}

void random_travel_time() {
	
	int i, j;
	
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			if (pointer_at_map->mappa[i][j].cell_type != 0) { 
				pointer_at_map->mappa[i][j].travel_time = (rand() % (SO_TIMENSEC_MAX - SO_TIMENSEC_MIN + 1)) + SO_TIMENSEC_MIN;
			}
		}
	}
}


/* La codifica è: 0 hole, 1 SO_SOURCES, 2 free */
void map_setup() {
	
	int i, j, max_taxi_map = 0, condizione_ok = 0, counter = 0; /* Counter mi serve per i vertici */

#ifdef MAPPA_VALORI_CASUALI
	random_cell_type();
	/* Controlli su mappe particolari */
	/* Mappe di una sola riga */
	if (SO_HEIGHT == 1 && SO_WIDTH > 1) {
		for(j = 1; j < SO_WIDTH - 1; j++){
			if (pointer_at_map->mappa[0][j].cell_type == 0) {
				printf("Errore: non posso avere celle holes in mezzo. Termino.\n");
				kill_all();
				exit(EXIT_FAILURE);
			}
		}
	}   
	/* Mappe di una sola colonna */
	if (SO_WIDTH == 1 && SO_HEIGHT > 1) {
		for(i = 1; i < SO_HEIGHT - 1; i++){
			if (pointer_at_map->mappa[i][0].cell_type == 0) {
				printf("Errore: non posso avere celle holes in mezzo. Termino.\n");
				kill_all();
				exit(EXIT_FAILURE);
			}
		}
	}
#endif

	do {
		random_taxi_capacity();
		for (i = 0; i < SO_HEIGHT; i++) {
			for (j = 0; j < SO_WIDTH; j++) {
				/* Calcolo il numero massimo di taxi sulla mappa con le capienze assegnate */
				max_taxi_map = max_taxi_map + pointer_at_map->mappa[i][j].taxi_capacity;
			}
		}
		/* Controllo che SO TAXI non sia > capacità di taxi massima della mappa */
		if (SO_TAXI > max_taxi_map) {
			printf("Errore: dovrei mettere %i taxi, ma la capeinza massima della mappa e' %i \n", SO_TAXI, max_taxi_map);
			printf("Assegno nuove capienze massime. Assicurarsi che il numero inserito non sia troppo alto per la mappa. [Possibile loop infinito]\n");
		} else condizione_ok = 1;
	} while (condizione_ok == 0); /* Potenziale loop infinito se i parametri sono volutamente sbagliati */
	
	random_travel_time();
	
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			/* Imposto ogni cella con active_taxis = 0, crossings = 0, numero di vertice */
			pointer_at_map->mappa[i][j].active_taxis = 0;
			pointer_at_map->mappa[i][j].crossings = 0;
			if (pointer_at_map->mappa[i][j].cell_type != 0) {
				pointer_at_map->mappa[i][j].vertex_number = counter;
				counter++;
			} else {
				pointer_at_map->mappa[i][j].vertex_number = -1;
			}
		}
	}

/* Se non e' definita random_taxi_capacity e random_travel_time assegnano valori anche a celle che non avrebbero bisogno */
#ifndef MAPPA_VALORI_CASUALI
	pointer_at_map->mappa[0][0].cell_type = 0;
	pointer_at_map->mappa[1][3].cell_type = 0;
	pointer_at_map->mappa[3][2].cell_type = 0;
	pointer_at_map->mappa[2][2].cell_type = 1;
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
#endif
}

void check_map(){

	int num_source = 0, num_holes = 0, num_msg_queues = 0;
	int i, j;
	
	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
			if (pointer_at_map->mappa[i][j].cell_type == 0){
				num_holes++;
			} else if (pointer_at_map->mappa[i][j].cell_type == 1 || pointer_at_map->mappa[i][j].cell_type == 2) {
				if (pointer_at_map->mappa[i][j].cell_type == 1) { 
					num_source++;
				}	
				if (pointer_at_map->mappa[i][j].message_queue_key != 0) {
					num_msg_queues++;
				} 
				if (pointer_at_map->mappa[i][j].taxi_capacity <= 0){
					perror("Ho una cella con capacità di taxi <= 0, termino. ");
					kill_all();
					exit(EXIT_FAILURE);
				}
				if (pointer_at_map->mappa[i][j].travel_time <= 0){
					perror("Ho una cella con tempo di attraversamento <= 0, termino. ");
					kill_all();
					exit(EXIT_FAILURE);
				}
				if (pointer_at_map->mappa[i][j].cell_type == 1 && pointer_at_map->mappa[i][j].message_queue_key == 0){
					perror("Ho una cella con key per la coda di messaggi 0, termino. ");
					kill_all();
					exit(EXIT_FAILURE);
				}
				if (pointer_at_map->mappa[i][j].reference_sem_number < 0) {
					perror("Ho una cella senza numero del semaforo di riferimento, termino. ");
					kill_all();
					exit(EXIT_FAILURE);
				}
				if (pointer_at_map->mappa[i][j].vertex_number == -1) {
					perror("Ho una cella libera con numero vertice di una hole, termino. ");
					kill_all();
					exit(EXIT_FAILURE);
				} 
			} 
		} 
	}

	if (num_holes != SO_HOLES) {
		printf("%i \n", num_holes);
		perror("Non ho abbastanza holes. Termino. ");
		kill_all();
		exit(EXIT_FAILURE);
	}
	if (num_source != SO_SOURCES) {
		perror("Non ho abbastanza source. Termino. ");
		kill_all();
		exit(EXIT_FAILURE);
	}
	if (num_msg_queues != SO_SOURCES) {
		perror("Non ho abbastanza code di messaggi. Termino. ");
		kill_all();
		exit(EXIT_FAILURE);
	}
}


void map_print() {
	
	int i, j;

	if (pointer_at_map == NULL){
		perror("Funzione map_print: non riesco ad accedere alla mappa. Termino.\n");
		kill_all();
		exit(EXIT_FAILURE);
	} 

	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			printf ("%i ", pointer_at_map->mappa[i][j].cell_type);
	
#ifdef STAMPA_VALORI_CELLA
			printf ("%i", pointer_at_map->mappa[i][j].taxi_capacity);
			printf ("%i", pointer_at_map->mappa[i][j].active_taxis);
			printf ("%i", pointer_at_map->mappa[i][j].travel_time);
			printf ("%i", pointer_at_map->mappa[i][j].crossings);
#endif
		}
		printf("\n");
	}

	/*
	printf("Stampo la matrice di vertici \n");
	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
			printf("%i ", pointer_at_map->mappa[i][j].vertex_number);
		}
		printf("\n");
	}
	*/
	printf("\n");
}

void create_index(void **m, int rows, int cols, size_t sizeElement){
	int i;  
	size_t sizeRow = cols * sizeElement;
	m[0] = m+rows;
	for(i=1; i<rows; i++){      
		m[i] = ((char *)m[i-1]+sizeRow);
	}
}

unsigned int sizeof_dm(int rows, int cols, size_t sizeElement){
    size_t size = rows * (sizeof(void *) + (cols * sizeElement));
    return size;
}

/* Aggiunge gli archi */
void addEdge(int ** pointer, int i, int j) {
	pointer[i][j] = 1;
}

void createAdjacencyMatrix(){

	int i, j;
	int ** pointer;
	size_t dimension = sizeof_dm(number_of_vertices, number_of_vertices, sizeof(int));

	if (pointer_at_map == NULL){
		perror("Master createAdjacencyMatrix: non riesco ad attaccarmi alla mappa. Termino \n");
		kill_all();
		exit(EXIT_FAILURE);
	}

	/* Creo il segmento di memoria condivisa per la matrice adiacente */
	adjacency_matrix_shm_id = shmget(IPC_PRIVATE, dimension, IPC_CREAT|0600);
	if (adjacency_matrix_shm_id < 0){
		perror("Master createAdjacencyMatrix: non riesco a creare la memoria condivisa. Termino\n");
		kill_all();
		exit(EXIT_FAILURE);
	}

	/* Mi attacco come master alla matrice per inizializzarla */
	pointer = shmat(adjacency_matrix_shm_id, NULL, SHM_FLG);
	if (pointer == NULL){
		perror("Master createAdjacencyMatrix: non riesco ad attaccarmi alla memoria condivisa con la matrice adiacente. Termino\n");
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

	/*
	printf("Stampo la matrice adiacente dopo l'inizializzazione \n");
	for (i = 0; i < number_of_vertices; i ++){
		for (j = 0; j < number_of_vertices; j ++){
			printf("%i ", pointer[i][j]);
		}
		printf("\n");
	}
	*/

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

	/* Imposto gli zeri ad INFINITY cosi non lo devo fare ogni volta nei processi TAXI */
	for (i = 0; i < number_of_vertices; i++){
		for (j = 0; j < number_of_vertices; j++){
			if (pointer[i][j] == 0){
				pointer[i][j] = INFINITY;
			}
		}
	}

	/* Salvo l'id della matrice adiacente per i taxi */
	adjacency_matrix_shm_id_execve = malloc(sizeof(int));
	if(adjacency_matrix_shm_id_execve == NULL){
		perror("Master createAdjacencyMatrix: non riesco a creare l'array per l'id della matrice adiancente. Termino\n");
		kill_all();
		exit(EXIT_FAILURE);
	}
    args_taxi[2] = adjacency_matrix_shm_id_execve;
    sprintf(adjacency_matrix_shm_id_execve, "%d", adjacency_matrix_shm_id);

}

void createIPC() {
	
	int i, j, counter;
	
	/* Path per la ftok */
	char *path = "/tmp";
	
	/* Creo la memoria condivisa che contiene la mappa */
	map_shm_id = shmget (IPC_PRIVATE, sizeof(map), SHM_FLG);
	if (map_shm_id == -1) {
		perror("Master createIPC: non riesco a creare la memoria condivisa. Termino\n");
		kill_all();
		exit(EXIT_FAILURE);
	}
	
	/* Mi attacco come master alla mappa */
	pointer_at_map = shmat(map_shm_id, NULL, SHM_FLG);
	if (pointer_at_map == NULL) {
		perror("Master createIPC: non riesco ad attaccarmi alla memoria condivisa con la mappa. Termino\n");
		kill_all();
		exit(EXIT_FAILURE);
	}
	
	/* Inizializziamo la mappa */
	map_setup();

	/* Preparo gli argomenti per la execve */
    map_shm_id_execve = malloc(sizeof(int));
    if (map_shm_id_execve == NULL){
    	perror("Master createIPC: non riesco a creare l'array per salvare l'id della mappa condivisa\n");
    	kill_all();
    	exit(EXIT_FAILURE);
    }
	args_source[1] = args_taxi[1] = map_shm_id_execve;
    sprintf(map_shm_id_execve, "%d", map_shm_id);

    /* Creo il semaforo mutex per l'assegnazione delle celle di SOURCE */
	source_sem_id = semget(SOURCE_SEM_KEY, 1, 0600 | IPC_CREAT);
	if (source_sem_id == -1){
		perror("Master createIPC: non riesco a generare il semaforo per l'assegnazione delle Source. Termino\n");
		kill_all();
		exit(EXIT_FAILURE);
	} 
	
	/* Imposto il semaforo con valore 1 -MUTEX */
	if (semctl(source_sem_id, 0, SETVAL, 1) == -1) {
		perror("Master createIPC: non riesco a impostare il semaforo per Source. Termino\n");
		kill_all();
		exit(EXIT_FAILURE);
	}
	
	/* Creo l'array di semafori per i TAXI, uno per ogni cella */
	taxi_sem_id = semget(TAXI_SEM_KEY, TAXI_SEM_ARRAY_DIM, 0600 | IPC_CREAT);
	if (taxi_sem_id == -1){
		perror("Master createIPC: non riesco a generare il semaforo per Taxi. Termino\n");
		kill_all();
		exit(EXIT_FAILURE);
	}

	/* Assegno il numero del semaforo Taxi di riferimento ad ogni cella */
	counter = 0;
	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
			if (pointer_at_map->mappa[i][j].cell_type != 0) {
				if(semctl(taxi_sem_id, counter, SETVAL, pointer_at_map->mappa[i][j].taxi_capacity) == -1 ){
					TEST_ERROR
					perror("Master createIPC: non riesco a impostare il semaforo per Taxi. Termino\n");
					kill_all();
					exit(EXIT_FAILURE);
				}
				pointer_at_map->mappa[i][j].reference_sem_number = counter;
				counter++;
			}
		}
	}
	
	/* Creiamo le code di messaggi per le celle source */
	pointer_at_msgq = malloc(SO_SOURCES*sizeof(key_t));
	if (pointer_at_msgq == NULL){
		perror("Master createIPC: non riesco a creare l'array per salvare le key delle code di messaggi. Termino\n");
		kill_all();
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < SO_SOURCES; i ++) {
		pointer_at_msgq[i] = ftok(path, i+1);
		TEST_ERROR
		if(msgget(pointer_at_msgq[i], 0600 | IPC_CREAT | IPC_EXCL) == -1) {
			perror("Master createIPC: non riesco a creare la coda di messaggi. Termino\n");
			TEST_ERROR
			kill_all();
			exit(EXIT_FAILURE);
		}
	}
	
	
	printf("STAMPA DI TEST DEL MASTER : STAMPO LE KEY CHE HO CREATO \n");
	for (i = 0; i < SO_SOURCES; i++){
		printf("%i \n", pointer_at_msgq[i]);
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

	printf("Stampo la mappa \n");
	map_print();
	printf("Stampo le celle dove ho la coda di messaggi \n");
	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
			if (pointer_at_map->mappa[i][j].message_queue_key != 0){
				printf("1 ");
			} else {
				printf("0 ");
			}
		}
	printf("\n");
	}

}

void kill_all() {
	/* Completare. Dovrà terminare le risorse IPC che allocheremo. */
	int msqid, i;
	
	/* Marco per la deallocazione la memoria condivisa con la mappa */
	shmctl(map_shm_id, IPC_RMID, NULL);

	/* Marco per la deallocazione la memoria condivisa con la matrice adiacente */
	shmctl(adjacency_matrix_shm_id, IPC_RMID, NULL);

	/* Dealloco il semaforo per Source */
	semctl(source_sem_id, 0, IPC_RMID);
	
	/* Dealloco l'array di semafori per Taxi */
	semctl(taxi_sem_id, 0, IPC_RMID);
	
	for (i = 0; i < SO_SOURCES; i++) {
		msqid = msgget(pointer_at_msgq[i], 0600);
		if (msqid < 0) {
			perror("Errore nella deallocazione \n");
		}
		msgctl(msqid , IPC_RMID , NULL);
	}
	
	if (pointer_at_msgq != NULL)free(pointer_at_msgq);

	if (pointer_at_msgq != NULL)free(adjacency_matrix_shm_id_execve);

	if (pointer_at_msgq != NULL)free(map_shm_id_execve);

	if (pointer_at_msgq != NULL)free(creation_moment);
	
	if (pointer_at_msgq != NULL)free(child_source);

	if (pointer_at_msgq != NULL)free(child_taxi);
}

#if 0
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

void ctrlc_handler(int signum) {
    printf("Ho ricevuto un control c \n");
    kill_all();
}
#endif
int main () {

	int i, j, valore_fork_sources, valore_fork_taxi;
	
    struct timeval time; 
	
    int created_at_start = 0;

    /* struct sigaction sa, sb; */
	
    gettimeofday(&time, NULL);
    srand((time.tv_sec * 1000) + (time.tv_usec)); 
	
	/* srand(time(NULL)); */
#if 0	
	bzero(&sa, sizeof(sa));
	sa.sa_handler = taxi_handler;
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);

    bzero(&sb, sizeof(sb));
    sb.sa_handler = ctrlc_handler;
    sb.sa_flags = 0;
    sigaction(SIGINT, &sb, NULL);
#endif
	/* Lettura degli altri parametri specificati da file */
	reading_input_values();

	/* Creo gli oggetti ipc */
	createIPC();

	/* Creo la matrice adiacente */
	createAdjacencyMatrix();

	/* Controlliamo che la mappa rispetti i valori inseriti */
	check_map();

	/* Creo l'array dove salvo le dimensione dei figli */
	child_source = calloc(SO_SOURCES, sizeof(pid_t));
	if (child_source == NULL){
		perror("Master main: non riesco a crare l'array dove salvare le informazioni sui processi Source. Termino \n");
		kill_all();
		exit(EXIT_FAILURE);
	}	
	child_taxi = calloc(SO_TAXI, sizeof(pid_t));
	if (child_taxi == NULL){
		perror("Master main: non riesco a crere l'array dove salvare le informazioni sui processi Taxi. Termino \n");
		kill_all();
		exit(EXIT_FAILURE);
	}
	
	/* Creo la stringa per informare il taxi su quando e' stato creato */
	creation_moment = malloc(sizeof(int));
	if (creation_moment == NULL){
		perror("Master main: non riesco a creare l'array dove salvare il momento di creazione del processo Taxi. Termino \n");
		kill_all();
		exit(EXIT_FAILURE);
	}
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
	/*
	printf("Ora dormo \n");
	sleep(5);
	*/
	/* Aspetto la terminazione dei figli */
	while(wait(NULL) != -1) {
	}
	
	printf("\n");
	printf("Stampo tutte le informazioni dei figli e se crasha \n");
	for (i = 0; i < SO_SOURCES; i++){
		printf("%i  ", child_source[i]);	
	}
	printf("\n");
	

	for (i = 0; i < SO_TAXI; i++){
		printf("%i  ", child_taxi[i]);
	}
	printf("\n");
	
	/*
	printf("IL NUMERO DI TAXI ATTIVI \n");
	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
			printf("%i   ", pointer_at_map->mappa[i][j].active_taxis);
		}
		printf("\n");
	}
	*/

	printf("\n");
	printf("Stampo la prima di finire \n");
	map_print();
	printf("\n");
	kill_all();
	return 0;
}
