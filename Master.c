#include <stdio.h>

#define SO_WIDTH 3
#define SO_HEIGHT 3
#define array_dimension 6
 
/* Variabili globali 
int riga;
int colonna;
*/
int array[array_dimension];
int* pointer = array;

struct array 
{
	int array[array_dimension];
};

struct map
{
	int riga;
	int colonna;
	int (*pointer)[array_dimension];
};


int** puntatoreAdArray() {
	/* int (*pointer)[array_dimension]; */
	return (&pointer);
}

void inizializzaMatrice (){
	
	int i = 0;
	int j = 0;
	int** matrice[SO_WIDTH][SO_HEIGHT];

	for (; i < SO_WIDTH; i++){
		for (; j < SO_HEIGHT; j++){
			int** puntatore;
			puntatore = puntatoreAdArray();
			matrice[i][j] = puntatore;
		}
	}
}

int main () {

	return 0;
}

