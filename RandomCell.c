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

#define SO_WIDTH 10
#define SO_HEIGHT 10

typedef struct 
{
	int cell_type;
} cell;

typedef struct 
{
	cell mappa[SO_HEIGHT][SO_WIDTH];
} map;

map mappa;
map *pointer_at_map = &mappa; 
int SO_HOLES = 8;
int SO_SOURCE = 5;

// l'idea è che prendo una mappa che inizializzo a un certo valore
// tirando a caso due coordinate inserisco gli hole con i relativi controlli
// tirando a caso due coordinate inserisco le celle source negli spazi rimanenti

void randomcell(map * pointer_at_map) {

    int holepiazzati = 0;
    int posizione;
    int sourcepiazzate = 0;
    //prima gli hole
    while (holepiazzati < SO_HOLES) {
        
        int x = rand() % ((SO_HEIGHT-1) - 0 + 1) + 0;
        int y = rand() % ((SO_WIDTH-1) - 0 + 1) + 0;

        //devo capire dove mi trovo
        if ((x == 0 && y == 0)||(x == 0 && y == SO_WIDTH-1)||(x == SO_HEIGHT-1 && y == 0)||(x == SO_HEIGHT-1 && y == SO_WIDTH -1)){
            if ((x == 0 && y == 0)) {
                posizione = 1; //angolo in alto a sinistra
            } else if ((x == 0 && y == SO_WIDTH-1)){
                posizione = 2; //angolo in alto a destra
            } else if ((x == SO_HEIGHT-1 && y == 0)) {
                posizione = 3; //angolo in basso a sinistra
            } else {
                posizione = 4; //angolo in basso a destra
            }
        } else if ((x == 0) || (y == 0) || (x == SO_HEIGHT -1) || (y == SO_WIDTH-1)){
            if (x == 0){
                posizione = 5; //bordo in alto
            } else if (y == 0) {
                posizione = 6; //bordo a sinistra
            } else if (x == SO_HEIGHT - 1) {
                posizione = 7; //bordo sotto
            } else {
                posizione = 8; //bordo a destra
            }
        } else {
            posizione = 9; //mezzo
        }
        
        //devo controllare le celle adiacenti in base a cosa vale 1
        switch (posizione)
        {
        case 1: //angolo in alto a sinistra
            if ((pointer_at_map->mappa[x][y+1].cell_type != 0 && pointer_at_map->mappa[x+1][y+1].cell_type != 0 && pointer_at_map->mappa[x+1][y].cell_type != 0) &&
                (pointer_at_map->mappa[x][y].cell_type != 0)) {
                pointer_at_map->mappa[x][y].cell_type = 0;
                holepiazzati++;
            }
            break;

        case 2: //angolo in alto a destra
            if ((pointer_at_map->mappa[x][y-1].cell_type != 0 && pointer_at_map->mappa[x+1][y-1].cell_type != 0 && pointer_at_map->mappa[x+1][y].cell_type != 0) &&
                (pointer_at_map->mappa[x][y].cell_type != 0)){
                pointer_at_map->mappa[x][y].cell_type = 0;
                holepiazzati++;
            }
            break;

        case 3: //angolo in basso a sinistra
            if ((pointer_at_map->mappa[x-1][y].cell_type != 0 && pointer_at_map->mappa[x-1][y+1].cell_type != 0 && pointer_at_map->mappa[x][y+1].cell_type != 0) &&
                (pointer_at_map->mappa[x][y].cell_type != 0)){
                pointer_at_map->mappa[x][y].cell_type = 0;
                holepiazzati++;
            }
            break;
        
        case 4: //angolo in basso a destra
            if ((pointer_at_map->mappa[x-1][y].cell_type != 0 && pointer_at_map->mappa[x-1][y-1].cell_type != 0 && pointer_at_map->mappa[x][y-1].cell_type != 0) &&
                (pointer_at_map->mappa[x][y].cell_type != 0)) {
                pointer_at_map->mappa[x][y].cell_type = 0;
                holepiazzati++;
            }
            break;

        case 5: //bordo in alto
            if ((pointer_at_map->mappa[x][y-1].cell_type != 0 && pointer_at_map->mappa[x+1][y-1].cell_type != 0 && pointer_at_map->mappa[x+1][y].cell_type != 0 
                && pointer_at_map->mappa[x+1][y+1].cell_type != 0 && pointer_at_map->mappa[x][y+1].cell_type != 0) && (pointer_at_map->mappa[x][y].cell_type != 0)) {
                    pointer_at_map->mappa[x][y].cell_type = 0;
                    holepiazzati++;
            }
            break;
        
        case 6: //bordo a sinistra
            if ((pointer_at_map->mappa[x-1][y].cell_type != 0 && pointer_at_map->mappa[x-1][y+1].cell_type != 0 && pointer_at_map->mappa[x][y+1].cell_type != 0 
                && pointer_at_map->mappa[x+1][y+1].cell_type != 0 && pointer_at_map->mappa[x+1][y].cell_type != 0) && (pointer_at_map->mappa[x][y].cell_type != 0)) {
                    pointer_at_map->mappa[x][y].cell_type = 0;
                    holepiazzati++;
            }
            break;
        
        case 7: //bordo sotto
            if ((pointer_at_map->mappa[x][y-1].cell_type != 0 && pointer_at_map->mappa[x-1][y-1].cell_type != 0 && pointer_at_map->mappa[x-1][y].cell_type != 0 
                && pointer_at_map->mappa[x-1][y+1].cell_type != 0 && pointer_at_map->mappa[x][y+1].cell_type != 0) && (pointer_at_map->mappa[x][y].cell_type != 0)) {
                    pointer_at_map->mappa[x][y].cell_type = 0;
                    holepiazzati++;
            }
            break;
        
        case 8: //bordo a destra
            if ((pointer_at_map->mappa[x-1][y].cell_type != 0 && pointer_at_map->mappa[x-1][y-1].cell_type != 0 && pointer_at_map->mappa[x][y-1].cell_type != 0
                && pointer_at_map->mappa[x+1][y-1].cell_type != 0 && pointer_at_map->mappa[x+1][y].cell_type != 0) && (pointer_at_map->mappa[x][y].cell_type != 0)) { 
                    pointer_at_map->mappa[x][y].cell_type = 0;
                    holepiazzati++;       
                }
            break;
        
        case 9: //mezzo
            if ((pointer_at_map->mappa[x-1][y-1].cell_type != 0 && pointer_at_map->mappa[x-1][y].cell_type != 0 && pointer_at_map->mappa[x-1][y+1].cell_type != 0
                && pointer_at_map->mappa[x][y+1].cell_type != 0 && pointer_at_map->mappa[x+1][y+1].cell_type != 0 && pointer_at_map->mappa[x+1][y].cell_type != 0 
                && pointer_at_map->mappa[x+1][y-1].cell_type != 0 && pointer_at_map->mappa[x][y-1].cell_type != 0) && (pointer_at_map->mappa[x][y].cell_type != 0)) {
                    pointer_at_map->mappa[x][y].cell_type = 0;
                    holepiazzati++;
                }
            break;
        
        default:
            break;
        }
    }

    //ora piazzo le source 
    while (sourcepiazzate < SO_SOURCE)
    {
        int x = rand() % ((SO_HEIGHT-1) - 0 + 1) + 0;
        int y = rand() % ((SO_WIDTH-1) - 0 + 1) + 0;
        if (pointer_at_map->mappa[x][y].cell_type != 0){
            pointer_at_map->mappa[x][y].cell_type = 1;
            sourcepiazzate++;
        }
    }
    printf("Ho posizionato %i hole \n", holepiazzati);
    printf("Ho posizionato %i source \n", sourcepiazzate);
}

int main() {
    
    srand(time(NULL)); //POTREBBE ESSERE ESATTAMENTE QUELLO CHE BINI HA DETTO DI NON FARE 

    for (int i = 0; i < SO_HEIGHT; i++){
        for (int j = 0; j < SO_WIDTH; j++){
            pointer_at_map->mappa[i][j].cell_type = 2;
        }
    }

    randomcell(pointer_at_map);

    printf("Test stampa mappa \n");
    for (int i = 0; i < SO_HEIGHT; i++){
        for (int j = 0; j < SO_WIDTH; j++){
            printf("%i  ", pointer_at_map->mappa[i][j].cell_type);
        }
        printf("\n");
    }
#if 0
    for (int i = SO_WIDTH*SO_HEIGHT; i > 0; i--){
        int x = rand() % ((SO_HEIGHT-1) - 0 + 1) + 0;
        int y = rand() % ((SO_WIDTH-1) - 0 + 1) + 0;
        printf("x: %i  y: %i \n", x, y);
    }
#endif
}
