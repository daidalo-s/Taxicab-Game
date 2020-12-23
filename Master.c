#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include <errno.h>   
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

/* Da modificare: dovrà leggere i parametri da file e con rand
 * impostare i vari campi della struct. Hole necessita di 
 * particolare attenzione per il discorso della generazione 
 * casuale (parlane con gli altri)
 * La codifica è: 0 hole, 1 no SO_SOURCE, 2 SO_SOURCES già presente
 */
void Map_Setup(int larghezza, int altezza, cell** map) {
	int i, j;
	for (i = 0; i < larghezza; i++) {
    	for (j = 0; j < altezza; j++) {
    		map[i][j].cell_type = 1;
    		/* Unico hole in mezzo alla mappa */
    		if (i == 1 && j == 1) map[i][j].cell_type = 0;
    	}
    }
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

int main () {
	/* Creazione e inizializzazione mappa */ 
  	cell** map = NULL;
  	map = Map_creation(larghezza, altezza, map);
  	Map_Setup(larghezza, altezza, map);
 	Map_print(map, larghezza, altezza);


	return 0;
}

