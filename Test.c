#include <stdio.h> 

#define larghezza 3
#define altezza 3
#define numerocampi 6

void stampaMappa (int mappa[larghezza][altezza][numerocampi]) {
    int i, j, k;
    for (i = 0; i < larghezza; i++){
        for (j = 0; j < altezza; j++){
            for (k = 0; k < numerocampi; k++){
                printf ("%i %i %i %i %i %i \n", numerocampi[i][j][k]);
            }
        }
    }
}

int main(int argc, char const *argv[])
{
    int mappa[larghezza][altezza][numerocampi];
    int i, j, k;
    for (i = 0; i < larghezza; i++){
        for (j = 0; j < altezza; j++){
            for (k = 0; k < numerocampi; k++){
                mappa[i][j][k] = 0;
            }
        }
    }
    stampaMappa(mappa);
    return 0;
}


