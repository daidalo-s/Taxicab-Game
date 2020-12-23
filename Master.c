#include <stdio.h>
#include <stdlib.h>
/* Prima o poi dovremo toglierle */
#define SO_WIDTH 3
#define SO_HEIGHT 3

typedef struct 
{
	int cell_type;
	int taxi_capacity;
	int active_taxis;
	int travel_time;
	int crossings;
} cell; 

/****************** Prototipi ******************/
cell** Map_creation(int larghezza, int altezza, cell** map);
void Map_print(cell** map, int larghezza, int altezza);
void Map_Setup(int larghezza, int altezza, cell** map);

/* ---------------- Variabili globali ----------------- */
/* Larghezza e altezza andranno sistemati */
int larghezza = 3;
int altezza = 3;


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
/* Dovrebbe andare */
void Map_print(cell** map, int larghezza, int altezza) {
	int i, j;
	for (i = 0; i < larghezza; i++) {
    	for (j = 0; j < altezza; j++) {
      		printf ("%i ", map[i][j].cell_type);
      		#if 0
      		printf ("%i", mappa[i][j].taxi_capacity);
      		printf ("%i", mappa[i][j].active_taxis);
     		printf ("%i", mappa[i][j].travel_time);
     		printf ("%i", mappa[i][j].crossings);
    		#endif
    	}
    	printf("\n");
  	}
}
/* Da modificare con la rand e i controlli per le hole */
void Map_Setup(int larghezza, int altezza, cell** map) {
	int i, j;
	for (i = 0; i < larghezza; i++) {
    	for (j = 0; j < altezza; j++) {
    		map[i][j].cell_type = 1;
    	}
    }
}

int main () {
	/* Creazione e inizializzazione mappa */ 
  	cell** map = NULL;
  	map = Map_creation(larghezza, altezza, map);
  	Map_Setup(larghezza, altezza, map);
 	Map_print(map, larghezza, altezza);


	return 0;
}

