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
#include "Map.h"

unsigned int sizeof_dm(int rows, int cols, size_t sizeElement){
    size_t size = rows * (sizeof(void *) + (cols * sizeElement));
    return size;
}

void create_index(void **m, int rows, int cols, size_t sizeElement){
    int i;  
    size_t sizeRow = cols * sizeElement;
    m[0] = m+rows;
    for(i=1; i<rows; i++){      
        m[i] = (m[i-1]+sizeRow);
    }
}

void print_matriz(int **matrix, int Rows, int Cols){
    int i, j;
    printf("\n");
        for(i=0; i<Rows; i++){
            for(j=0; j<Cols; j++)
                printf("%.2d\t",matrix[i][j]);
            printf("\n");
        }
}

int main(int argc, char **argv){

    int **matrix;
    int i, j;
    int Cols = 10, Rows = 5, shmId;

    size_t sizeMatrix = sizeof_dm(Rows,Cols,sizeof(int));
    shmId = shmget(IPC_PRIVATE, sizeMatrix, IPC_CREAT|0600);    
    matrix = shmat(shmId, NULL, 0); 
    create_index((void*)matrix, Rows, Cols, sizeof(int));


    if(fork()){
        int pos=0;
        for(i=0; i<Rows; i++){
            for(j=0; j<Cols; j++)
                matrix[i][j] = pos++;
        }       
        wait(NULL); 
        print_matriz(matrix, Rows, Cols); /*verify child's write*/
        shmdt(matrix);
        shmctl(shmId, IPC_RMID, 0);
    }
    else{   
        sleep(3); /*avoid race condition*/
        print_matriz(matrix, Rows, Cols);
        matrix[2][2] = 9; /*writing test from child*/
        shmdt(matrix);
    }
    return 0;
}
