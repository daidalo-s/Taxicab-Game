#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include <errno.h> 
#include <time.h>  
/* Ogni volta che le attivo al compilatore non piacciono */
#if 0
#define SO_WIDTH 3
#define SO_HEIGHT 3
#endif

typedef struct 
{
	int cell_type;
	int taxi_capacity;
	int active_taxis;
	int travel_time;
	int crossings;
} cell; 

/****************** Prototipi ******************/
void Reading_Input_Values (); 
cell** Map_creation(int larghezza, int altezza, cell** map);
void Map_print(int larghezza, int altezza, cell** map);
void Map_Setup(int larghezza, int altezza, cell** map);

/* ---------------- Variabili globali ----------------- */
cell** map = NULL;
/* Larghezza e altezza andranno sistemati */
int larghezza = 3;
int altezza = 3;
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
void Reading_Input_Values () {

	char tmpstr1[16];
  char tmpstr2[16];
  char tempbuff[100];

  FILE *input = fopen("Parameters.txt", "r");
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
      else{
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

/* Crea la mappa e la restituisce al main */
cell** Map_creation(int larghezza, int altezza, cell** map) {
	int i;
	map = malloc(larghezza * sizeof(cell));
	for (i = 0; i < larghezza; i++){
		map[i] = calloc(altezza, sizeof(cell));
	}
	return map;
}

/* Da modificare: dovrà leggere i parametri da file e con rand
 * impostare i vari campi della struct. Hole necessita di 
 * particolare attenzione per il discorso della generazione 
 * casuale (parlane con gli altri)
 * Se la mappa generata non è corretta si termina con errore
 * La codifica è: 0 hole, 1 no SO_SOURCE, 2 cella libera,
 * 3 SO_SOURCES già presente
 */
void Map_Setup(int larghezza, int altezza, cell** map) {
	int i, j;
  srand(time(0));
	for (i = 0; i < larghezza; i++) {
    	for (j = 0; j < altezza; j++) {
        /* https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand 
           rand() % (2+1-0) + 0; */
    		map[i][j].cell_type = Random_Cell_Type(map, i, j);
        map[i][j].taxi_capacity = Random_Taxi_Capacity();
        map[i][j].active_taxis = 0;
        map[i][j].travel_time = Random_Travel_Time();
    		/* Unico hole in mezzo alla mappa */
    		if (i == 1 && j == 1) map[i][j].cell_type = 0;
    	}
    }
}

/* Dovrebbe andare */
void Map_print(int larghezza, int altezza, cell** map) {
	int i, j;
	for (i = 0; i < larghezza; i++) {
    	for (j = 0; j < altezza; j++) {
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

int main () {

	/* Lettura parametri */
 	Reading_Input_Values();

	/* Creazione e inizializzazione mappa DA SISTEMARE COI PARAMETRI 
	 * CHE LEGGO */ 
  map = Map_creation(larghezza, altezza, map);
  Map_Setup(larghezza, altezza, map);
 	Map_print(larghezza, altezza, map);

	return 0;
}

