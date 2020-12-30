/*Stiamo importando tutte le librerie necessarie?*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include <errno.h> 
#include <time.h>  
#include <unistd.h> 
#if 1
#include <sys/shm.h>
#define SO_HEIGHT 4 
#define SO_WIDTH 5
#endif

#if 1
typedef struct 
{
	int cell_type;
	int taxi_capacity;
	int active_taxis;
	int travel_time;
	int crossings;
} cell;

typedef struct 
{
	cell mappa[SO_HEIGHT][SO_WIDTH];
} map;
#endif
int main(int argc, char *argv[])
{
	int i, j;
	#if 1
	map lamiamappadiocane;
	map *lamiamappadiocan = &lamiamappadiocane;
	int shmid; 
	shmid = atoi(argv[1]);
	lamiamappadiocan = shmat(shmid, NULL, 0);
	#endif
    /* code */
    printf("Sono un processo SO_SOURCE \n");
    printf("Questa e' una stampa di un miracolo di SO_SOURCE \n");
    #if 1
    printf("Questa e' la stampa della mappa secondo SO_SOURCE \n");
    for (i = 0; i < SO_HEIGHT; i++){
    	for (j = 0; j < SO_WIDTH; j++){
    		printf("%i\n", lamiamappadiocan->mappa[i][j].cell_type);
    	}
    }
    printf("Il campo della cella 2.2 e': %i \n", lamiamappadiocan->mappa[2][2].cell_type);
    #endif
#if 0
    printf("%i \n", map[2][2].cell_type);
#endif
    printf("Ora perdo un po' di tempo e poi esco \n");
    sleep(10);
    printf("Ho finito di dormire \n");
    return 0;
}
