#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/types.h> 
#include <errno.h> 
#include <time.h>  
#include <unistd.h> 
#define MAPPA_VALORI_CASUALI 
/* Struttura cella */
typedef struct 
{
	int cell_type;
	int taxi_capacity;
	int active_taxis;
	int travel_time;
	int crossings;
} cell; 
/* Vettore dove tengo informazioni sui processi*/
typedef struct 
{
    pid_t pid;
} process_info;

/****************** Prototipi ******************/
void kill_all();
void reading_input_values (); 
cell** map_creation(int SO_WIDTH, int SO_HEIGHT, cell** map);
void free_map(cell** map);
void map_print(int SO_WIDTH, int SO_HEIGHT, cell** map);
void map_setup(int SO_WIDTH, int SO_HEIGHT, cell** map);
void Random_Cell_Type(cell** map);
void Random_Taxi_Capacity(cell** map);
void Random_Travel_Time(cell** map);

/* ---------------- Variabili globali ----------------- */
cell** map = NULL;
/* SO_WIDTH e SO_HEIGHT vengono letti dal main */
int SO_WIDTH = 0;
int SO_HEIGHT = 0;
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

/* ---------------- Lettura parametri da file ----------------- */
void reading_input_values () {

	char tmpstr1[16];
	char tmpstr2[16];
	char tempbuff[100];
	FILE *input = fopen("Parameters.txt", "r");
#if 0
	printf("Inizializzazione della simulazione \n");
	printf("Inserire la larghezza della mappa: \n");
	scanf("%i", &SO_WIDTH);
	printf("Inserire l'altezza della mappa: \n");
	scanf("%i", &SO_HEIGHT);
#endif
	if (input == NULL) {
		printf ("Errore, non riesco ad aprire il file \n");
		exit(-1); /* oppure return -1 */
	}

	while(!feof(input)) {

		if (fgets(tempbuff,100,input)) {
			sscanf(tempbuff, "%15s = %15[^;];", tmpstr1, tmpstr2);

			if (strcmp(tmpstr1,"SO_HOLES")==0) {
				SO_HOLES = atoi(tmpstr2);
				if (SO_HOLES < 1) {
					printf("Errore, parametro SO_HOLES non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}       
			else if (strcmp(tmpstr1,"SO_TOP_CELLS")==0) {
				SO_TOP_CELLS = atoi(tmpstr2);
				if (SO_TOP_CELLS < 1) {
					printf("Errore, parametro SO_TOP_CELLS non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_SOURCES")==0) {
				SO_SOURCES = atoi(tmpstr2);
				if (SO_SOURCES < 1) {
					printf("Errore, parametro SO_SOURCES non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_CAP_MIN")==0) {
				SO_CAP_MIN = atoi(tmpstr2);
				if (SO_CAP_MIN < 0) {
					printf("Errore, parametro SO_CAP_MIN non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}  
			else if (strcmp(tmpstr1,"SO_CAP_MAX")==0) {
				SO_CAP_MAX = atoi(tmpstr2);
				if (SO_CAP_MAX < SO_CAP_MIN){
					printf("Errore, parametro SO_CAP_MAX non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_TAXI")==0) {
				SO_TAXI = atoi(tmpstr2);
				if (SO_TAXI < 0){
					printf("Errore, parametro SO_TAXI non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_TIMENSEC_MIN")==0) {
				SO_TIMENSEC_MIN = atoi(tmpstr2);
				if (SO_TIMENSEC_MIN < 0) {
					printf("Errore, parametro SO_TIMENSEC_MIN non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_TIMENSEC_MAX")==0) {
				SO_TIMENSEC_MAX = atoi(tmpstr2);
				if (SO_TIMENSEC_MAX < SO_TIMENSEC_MIN){
					printf("Errore, parametro SO_TIMENSEC_MAX non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_TIMEOUT")==0) {
				SO_TIMEOUT = atoi(tmpstr2);
				if (SO_TIMEOUT < 0) {
					printf("Errore, parametro SO_TIMEOUT non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_DURATION")==0) {
				SO_DURATION = atoi(tmpstr2);
				if (SO_DURATION < SO_TIMEOUT) {
					printf("Errore, parametro SO_TIMEOUT non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else {
				printf("Parametro non riconosciuto : \"%s\"\n", tmpstr1);
			}
		}
	}
	fclose(input);

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
}

/* ---------------- Metodi mappa ----------------- */
#ifdef MAPPA_VALORI_CASUALI
/* Inizializza cell_type in modo casuale */
void Random_Cell_Type(cell** map) {
    /* Variabili locali utilizzate:
     * - value assume il valore della codifica (0 hole, 1 no SO_SOURCES, 2 cella libera) 
     *   da assegnare alla cella[i][j]; i e j assumono il valore degli indici della cella di riferimento;
     * - row_pos (row position) e col_pot (column position) sono variabili utilizzate 
     *   per capire in seguito quale case eseguire all'interno degli switch;
     * - so_holes e so_sources assumono i valori delle variabili SO_HOLES ed SO_SOURCES,
     *   dato che quest'ultime sono variabili globali e' meglio non modificarne il valore
     *   ma bensi' utilizzarne una copia all'interno della funzione
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
            map[i][j].cell_type = value;
        } else {    
            switch (row_pos) {
                case 0:
                    if (map[i][j-1].cell_type == 0) { 
                        do {
                        value = rand() % (2-1+1) + 1;
                        } while (value == 1 && so_sources == 0);
                        if (value == 1) so_sources = so_sources-1;
                        map[i][j].cell_type = value;
                    } else {
                        do {
                        value = rand() % (2-0+1) + 0;
                        } while ((value == 0 && so_holes == 0) || (value == 1 && so_sources == 0));
                        if (value == 0) so_holes = so_holes-1;
                        if (value == 1) so_sources = so_sources-1;
                        map[i][j].cell_type = value;
                    }
                    break;
                case 1:
                    switch (col_pos) {
                        case 0:
                            if ((map[i-1][j].cell_type != 0) && (map[i-1][j+1].cell_type != 0)) {
                                do {
                                value = rand() % (2-0+1) + 0;
                                } while ((value == 0 && so_holes == 0) || (value == 1 && so_sources == 0));
                                if (value == 0) so_holes = so_holes-1;
                                if (value == 1) so_sources = so_sources-1;
                                map[i][j].cell_type = value;
                            } else {
                                do {
                                value = rand() % (2-1+1) + 1;
                                } while (value == 1 && so_sources == 0);
                                if (value == 1) so_sources = so_sources-1;
                                map[i][j].cell_type = value;
                            }
                            break;
                        case 1:
                            if ((map[i][j-1].cell_type != 0) && (map[i-1][j-1].cell_type != 0) &&
                                        (map[i-1][j].cell_type != 0) && (map[i-1][j+1].cell_type != 0)) {
                                do {
                                value = rand() % (2-0+1) + 0;
                                } while ((value == 0 && so_holes == 0) || (value == 1 && so_sources == 0));
                                if (value == 0) so_holes = so_holes-1;
                                if (value == 1) so_sources = so_sources-1;
                                map[i][j].cell_type = value;
                            } else {
                                do {
                                value = rand() % (2-1+1) + 1;
                                } while (value == 1 && so_sources == 0);
                                if (value == 1) so_sources = so_sources-1;
                                map[i][j].cell_type = value;
                            }
                            break;
                        case 2:
                            if ((map[i][j-1].cell_type != 0) && (map[i-1][j-1].cell_type != 0) &&
                                    (map[i-1][j].cell_type != 0)) {
                                do {
                                value = rand() % (2-0+1) + 0;
                                } while ((value == 0 && so_holes == 0) || (value == 1 && so_sources == 0));
                                if (value == 0) so_holes = so_holes-1;
                                if (value == 1) so_sources = so_sources-1;
                                map[i][j].cell_type = value;
                            } else {
                                do {
                                value = rand() % (2-1+1) + 1;
                                } while (value == 1 && so_sources == 0);
                                if (value == 1) so_sources = so_sources-1;
                                map[i][j].cell_type = value;
                            }
                            break;
                        default:
                            printf("Errore\n");
                            exit(-1);
                    }
                    break;
                default:
                    printf("Errore\n");
                    exit(-1);
            }   
        }
    } 
    }
}

/* Nei casi in cui si odvesse verificare qualche anomalia viene restituito 1, 
 * ma per generare un errore cosa possiamo fare?
 */

/* Assegna ad ogni cella taxi_capacity*/
void Random_Taxi_Capacity(cell** map) {
    int i, j;
    for (i = 0; i < SO_HEIGHT; i++) {
            for (j = 0; j < SO_WIDTH; j++) {
                map[i][j].taxi_capacity = (rand() % (SO_CAP_MAX - SO_CAP_MIN + 1)) + SO_CAP_MIN;
            }
        }
}

/* Assegna ad ogni cella travel_time*/
void Random_Travel_Time(cell** map) {
    int i, j;
    for (i = 0; i < SO_HEIGHT; i++) {
            for (j = 0; j < SO_WIDTH; j++) {
                map[i][j].travel_time = (rand() % (SO_TIMENSEC_MAX - SO_TIMENSEC_MIN + 1)) + SO_TIMENSEC_MIN;
            }
        }
}

#endif

/* Crea la mappa e la restituisce al main */
cell** map_creation(int SO_WIDTH, int SO_HEIGHT, cell** map) {

	int i;
	map = malloc(SO_WIDTH * sizeof(cell));
	for (i = 0; i < SO_WIDTH; i++){
		map[i] = calloc(SO_HEIGHT, sizeof(cell));
	}
	return map;
}

/* Da modificare: dovrà leggere i parametri da file e con rand
 * impostare i vari campi della struct. 
 * Se la mappa generata non è corretta si termina con errore
 * La codifica è: 0 hole, 1 SO_SOURCE, 2 no SO_SORUCE
 * Al momento usiamo una mappa 5x4 (quindi non importa quello che date
 * al programma in input che larghezza e altezza, ne faccio override nel
 * main) se volete lo schema della mappa che sto usando lo trovate nel 
 * documento condiviso
 */
void map_setup(int SO_WIDTH, int SO_HEIGHT, cell** map) {

	int i, j;
	/* È giusto inizializzarla così? */
	srand(getpid());
	/* Inizializzo la mappa con valore 2 */
	for (i = 0; i < SO_WIDTH; i++) {
		for (j = 0; j < SO_HEIGHT; j++) {
			map[i][j].cell_type = 2;
            map[i][j].active_taxis = 0;
		}
	}
	/* Creo la mappa come nel documento condiviso */
	#if 0
    map[0][0].cell_type = 0;
	map[3][1].cell_type = 0;
	map[2][3].cell_type = 0;
	map[2][2].cell_type = 1;
	map[2][2].taxi_capacity = 0;
	map[2][2].active_taxis = 0;
	map[2][2].travel_time = 0;
    #endif
	/* https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand 
	   rand() % (2+1-0) + 0; */
#ifdef MAPPA_VALORI_CASUALI
	Random_Cell_Type(map);
    Random_Taxi_Capacity(map);
    /* ricontrolare 
    active_taxis = 0;*/
    Random_Travel_Time(map);
#endif
}

/* Dovrebbe andare */
void map_print(int SO_WIDTH, int SO_HEIGHT, cell** map) {
	int i, j;
#if 0
	printf("La larghezza della mappa è: %i\n", SO_WIDTH);
#endif
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			printf ("%i ", map[i][j].cell_type);

#ifdef STAMPA_VALORI_CELLA
			printf ("%i", mappa[i][j].taxi_capacity);
			printf ("%i", mappa[i][j].active_taxis);
			printf ("%i", mappa[i][j].travel_time);
			printf ("%i", mappa[i][j].crossings);
#endif

		}
		printf("\n");
	}
}
/* Libera la mappa */
void free_map(cell** map) {
	int i;
	for (i = 0; i < SO_WIDTH; i++){
		free(map[i]);
	}
	free(map);
}

/* Chiusura di tutti i processi */
void kill_all() {
    /* Dobbiamo bloccare i segnali ke ankora nn abbiamo okkio*/

    /* Per terminare i processi scorriamo la matrice e appena ne
       troviamo uno zap */

}

/* Main */
int main () {
    #if 0
	int i, j, valore_fork_sources, valore_fork_taxi;
    #endif
	/* Lettura degli altri parametri specificati da file */
	reading_input_values();
	/* Creazione e inizializzazione mappa */
	SO_WIDTH = 5;
	SO_HEIGHT = 4;
	map = map_creation(SO_WIDTH, SO_HEIGHT, map);
	map_setup(SO_WIDTH, SO_HEIGHT, map);
    #if 0
	/* Creo processi SO_SOURCES*/
	for (i = 0; i < SO_SOURCES; i++) {
		switch(valore_fork_sources = fork()) {
			case -1:
				printf("Errore nella fork. Esco.\n");
				/*shutdown*/
				break;
			case 0:
                break;
				/* execve(); */
		}
	}

	/* Creo processi Taxi */
	for (j = 0; j < SO_TAXI; j++) {
		switch(valore_fork_taxi = fork()) {
			case -1:
				printf("Errore nella fork. Esco.\n");
				/*shutdown*/
				break;
			case 0:
                break;
				/* execve(); */
		}
	}
    #endif
	map_print(SO_WIDTH, SO_HEIGHT, map);
	free_map(map);
	return 0;
}
