#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include <errno.h> 
#include <time.h>  
#include <unistd.h>  
typedef struct 
{
	int cell_type;
	int taxi_capacity;
	int active_taxis;
	int travel_time;
	int crossings;
} cell; 

/****************** Prototipi ******************/
void reading_input_values (); 
cell** map_creation(int SO_WIDTH, int SO_HEIGHT, cell** map);
void free_map(cell** map);
void map_print(int SO_WIDTH, int SO_HEIGHT, cell** map);
void map_setup(int SO_WIDTH, int SO_HEIGHT, cell** map);
int random_cell_type(cell** map, int i, int j);
int random_taxi_capacity();
int random_travel_time();

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

	printf("Inizializzazione della simulazione \n");
	printf("Inserire la larghezza della mappa: \n");
	scanf("%i", &SO_WIDTH);
	printf("Inserire l'altezza della mappa: \n");
	scanf("%i", &SO_HEIGHT);

	if (input == NULL) {
		printf ("Errore, non riesco ad aprire il file \n");
		exit(-1); /* oppure return -1 */
	}

	while(!feof(input)) {

		if (fgets(tempbuff,100,input)) {
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
	/* Aggiungere logica per il controllo dei parametri inseriti */
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
int random_cell_type(cell** map, int i, int j) {
	/* c assumera' il valore della codifica da restituire (0 hole, 1 no SO_SOURCES, 2 cella libera),
	 * nei seguenti rami if-else si valuta invece quale valore assegnare ad x ed y per capire in seguito
	 * quale case eseguire all'interno degli switch (questo risolve il problema di "case i>0:")
	 */
	int c, x, y;
	static int so_holes;
	static int so_sources;
	so_holes = SO_HOLES;
	so_sources = SO_SOURCES;
	if (i == 0) { x = 0; } else x = 1;
	if (j == 0) { y = 0; } else if (j > 0 && j < (SO_WIDTH-1)) { y = 1; } else y = 2;
	/* In questo if seguente si esamina il caso in cui si e' alla prima cella controllando
	 * so_holes e so_sources per verificare che si possano inserire altri "buchi" o processi che
	 * potrebbero richiedere taxi
	 */
	if (i == 0 && j == 0) {
		do {
			c = rand() % (2-0+1) + 0;
		} while ((c == 0 && so_holes == 0) || (c == 1 && so_sources == 0));
		if (c == 0) so_holes--;
		if (c == 1) so_sources--;
		return c;
	}
	/* I case che fanno parte di "switch (x)" verificano cosa fare in base all'indice della riga
	 * che e' stata opportunamente codifica alla riga 11 del medesimo file
	 */
	switch (x) {
		case 0:
			if (map[i][j-1].cell_type == 0) { 
				if (so_sources == 0) {
					c = 2;
				} else {
					c = rand() % (2-1+1) + 1;
					if (c == 1) so_sources--;
				}
			} else {
				do {
					c = rand() % (2-0+1) + 0;
				} while ((c == 0 && so_holes == 0) || (c == 1 && so_sources == 0));
				so_sources--;
			}
			break;
		case 1:
			/* la stessa cosa spiegata alle righe 25-26 vale anche per "switch (y)", prima di 
			 * questo switch e' stata fatta un'opportuna codifica alla riga 12 considerando
			 * l'indice delle colonne.
			 * Ci sono tre case in quanto si devono controllare piu' o meno celle precedenti
			 * in base alla posizione dell'indice delle colonne appunto.
			 */
			switch (y) {
				case 0:
					if ((map[i-1][j].cell_type != 0) && (map[i-1][j+1].cell_type != 0)) {
						do {
							c = rand() % (2-0+1) + 0;
						} while ((c == 0 && so_holes == 0) || (c == 1 && so_sources == 0));
						if (c == 0) so_holes--;
						if (c == 1) so_sources--;
					} else {
						do {
							c = rand() % (2-1+1) + 1;
						} while (c == 1 && so_sources == 0);
						if (c == 1) so_sources--;
					}
					break;
				case 1:
					if ((map[i][j-1].cell_type != 0) && (map[i-1][j-1].cell_type != 0) &&
							(map[i-1][j].cell_type != 0) && (map[i-1][j+1].cell_type != 0)) {
						do {
							c = rand() % (2-0+1) + 0;
						} while ((c == 0 && so_holes == 0) || (c == 1 && so_sources == 0));
						if (c == 0) so_holes--;
						if (c == 1) so_sources--;
					} else {
						do {
							c = rand() % (2-1+1) + 1;
						} while (c == 1 && so_sources == 0);
						if (c == 1) so_sources--;
					}
					break;
				case 2:
					if ((map[i][j-1].cell_type != 0) && (map[i-1][j-1].cell_type != 0) &&
							(map[i-1][j].cell_type != 0)) {
						do {
							c = rand() % (2-0+1) + 0;
						} while ((c == 0 && so_holes == 0) || (c == 1 && so_sources == 0));
						if (c == 0) so_holes--;
						if (c == 1) so_sources--;
					} else {
						do {
							c = rand() % (2-1+1) + 1;
						} while ((c == 1 && so_sources == 0) || c == 0);
						if (c == 1) so_sources--;
					}
					break;
				default:
					return 1;
			}
			break;
		default:
			return 1;
	}
	return c;
}
/* Nei casi in cui si odvesse verificare qualche anomalia viene restituito 1, 
 * ma per generare un errore cosa possiamo fare?
 */

/* Assegna ad ogni cella taxi_capacity*/
int random_taxi_capacity() {
	return (rand() % (SO_CAP_MAX - SO_CAP_MIN + 1)) + SO_CAP_MIN;
}

/* Assegna ad ogni cella travel_time*/
int random_travel_time() {
	return (rand() % (SO_TIMENSEC_MAX - SO_TIMENSEC_MIN + 1)) + SO_TIMENSEC_MIN;
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
	srand(time(0));
	/* Inizializzo la mappa con valore 2 */
	for (i = 0; i < SO_WIDTH; i++) {
		for (j = 0; j < SO_HEIGHT; j++) {
			map[i][j].cell_type = 2;
		}
	}
	/* Creo la mappa come nel documento condiviso */
	map[0][0].cell_type = 0;
	map[3][1].cell_type = 0;
	map[2][3].cell_type = 0;
	map[2][2].cell_type = 1;
	map[2][2].taxi_capacity = 0;
	map[2][2].active_taxis = 0;
	map[2][2].travel_time = 0;
	/* https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand 
	   rand() % (2+1-0) + 0; */
#ifdef MAPPA_VALORI_CASUALI
	map[i][j].cell_type = random_cell_type(map, i, j);
	map[i][j].taxi_capacity = random_taxi_capacity();
	map[i][j].active_taxis = 0;
	map[i][j].travel_time = random_travel_time();
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

void free_map(cell** map) {
    int i;
    for (i = 0; i < SO_WIDTH; i++){
        free(map[i]);
    }
    free(map);
}


int main () {
	/* Lettura degli altri parametri specificati da file */
	reading_input_values();
	/* Creazione e inizializzazione mappa */
	SO_WIDTH = 5;
	SO_HEIGHT = 4; 
	map = map_creation(SO_WIDTH, SO_HEIGHT, map);
	map_setup(SO_WIDTH, SO_HEIGHT, map);
	map_print(SO_WIDTH, SO_HEIGHT, map);
  free_map(map);
  printf("Ciao");
	return 0;
}
