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
/****************** Prototipi ******************/
void kill_all();
void reading_input_values (); 
void random_cell_type(map *pointer_at_map);
void random_taxi_capacity(map *pointer_at_map);
void random_travel_time(map *pointer_at_map);
void map_creation(map *pointer_at_map);
void map_print(map *pointer_at_map);
void map_setup(map *pointer_at_map);
void free_map(map *pointer_at_map);

/* ---------------- Variabili globali ----------------- */
/* SO_WIDTH e SO_HEIGHT sono delle define im map.h */
map mappa;
map *pointer_at_map = &mappa;  
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
/* Variabili per la gestione della mappa*/
/* Argomenti da passare alla execve */
char * args_a[] = {"Source", NULL, NULL, NULL};
char * args_b[] = {"Taxi", NULL, NULL};
char m_id_str[4];
int shm_id; /* valore ritornato da shmget() */
int sem_id; /* valore ritornato da semget() */
int * pointer_at_msgq; 

/* ---------------- Lettura parametri da file ----------------- */
void reading_input_values () {

    char tmpstr1[16];
    char tmpstr2[16];
    char tempbuff[100];
    FILE *input = fopen("Parameters.txt", "r");

    if (input == NULL) {
        printf ("Errore, non riesco ad aprire il file \n");
        exit(EXIT_FAILURE); /* oppure return -1 */
    }

    while(!feof(input)) {

        if (fgets(tempbuff, sizeof(tempbuff),input)) {
            sscanf(tempbuff, "%15s = %15[^;];", tmpstr1, tmpstr2);

            if (strcmp(tmpstr1,"SO_HOLES")==0) {
                SO_HOLES = atoi(tmpstr2);
                if (SO_HOLES < 1) {
                    printf("Errore, parametro SO_HOLES non valido. Esco.\n");
                    fclose(input);
                    exit(EXIT_FAILURE);
                }
            }       
            else if (strcmp(tmpstr1,"SO_TOP_CELLS")==0) {
                SO_TOP_CELLS = atoi(tmpstr2);
                if (SO_TOP_CELLS < 1) {
                    printf("Errore, parametro SO_TOP_CELLS non valido. Esco.\n");
                    fclose(input);
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(tmpstr1,"SO_SOURCES")==0) {
                SO_SOURCES = atoi(tmpstr2);
                if (SO_SOURCES < 1) {
                    printf("Errore, parametro SO_SOURCES non valido. Esco.\n");
                    fclose(input);
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(tmpstr1,"SO_CAP_MIN")==0) {
                SO_CAP_MIN = atoi(tmpstr2);
                if (SO_CAP_MIN < 0) {
                    printf("Errore, parametro SO_CAP_MIN non valido. Esco.\n");
                    fclose(input);
                    exit(EXIT_FAILURE);
                }
            }  
            else if (strcmp(tmpstr1,"SO_CAP_MAX")==0) {
                SO_CAP_MAX = atoi(tmpstr2);
                if (SO_CAP_MAX < SO_CAP_MIN){
                    printf("Errore, parametro SO_CAP_MAX non valido. Esco.\n");
                    fclose(input);
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(tmpstr1,"SO_TAXI")==0) {
                SO_TAXI = atoi(tmpstr2);
                if (SO_TAXI < 0){
                    printf("Errore, parametro SO_TAXI non valido. Esco.\n");
                    fclose(input);
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(tmpstr1,"SO_TIMENSEC_MIN")==0) {
                SO_TIMENSEC_MIN = atoi(tmpstr2);
                if (SO_TIMENSEC_MIN < 0) {
                    printf("Errore, parametro SO_TIMENSEC_MIN non valido. Esco.\n");
                    fclose(input);
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(tmpstr1,"SO_TIMENSEC_MAX")==0) {
                SO_TIMENSEC_MAX = atoi(tmpstr2);
                if (SO_TIMENSEC_MAX < SO_TIMENSEC_MIN){
                    printf("Errore, parametro SO_TIMENSEC_MAX non valido. Esco.\n");
                    fclose(input);
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(tmpstr1,"SO_TIMEOUT")==0) {
                SO_TIMEOUT = atoi(tmpstr2);
                if (SO_TIMEOUT < 0) {
                    printf("Errore, parametro SO_TIMEOUT non valido. Esco.\n");
                    fclose(input);
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(tmpstr1,"SO_DURATION")==0) {
                SO_DURATION = atoi(tmpstr2);
                if (SO_DURATION < SO_TIMEOUT) {
                    printf("Errore, parametro SO_TIMEOUT non valido. Esco.\n");
                    fclose(input);
                    exit(EXIT_FAILURE);
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
void random_cell_type(map *pointer_at_map) {
    /* Variabili locali utilizzate:
     * - value assume il valore della codifica (0 hole, 1 no SO_SOURCES, 2 cella libera) 
     *   da assegnare alla cella[i][j]; 
     * - i e j assumono il valore degli indici della cella di riferimento;
     * - row_pos (row position) e col_pot (column position) sono variabili utilizzate 
     *   per capire in seguito quale case eseguire all'interno degli switch;
     * - so_holes e so_sources assumono i valori delle variabili SO_HOLES ed SO_SOURCES,
     *   dato che quest'ultime sono variabili globali, una scelta implementativa migliore
     *   e' di non modificarne il valore ma bensi' utilizzarne una copia 
     *   all'interno della funzione in questione
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
                pointer_at_map->mappa[i][j].cell_type = value;
            } else {    
                switch (row_pos) {
                    case 0:
                        if (pointer_at_map->mappa[i][j-1].cell_type == 0) { 
                            do {
                                value = rand() % (2-1+1) + 1;
                            } while (value == 1 && so_sources == 0);
                            if (value == 1) so_sources = so_sources-1;
                            pointer_at_map->mappa[i][j].cell_type = value;
                        } else {
                            do {
                                value = rand() % (2-0+1) + 0;
                            } while ((value == 0 && so_holes == 0) || (value == 1 && so_sources == 0));
                            if (value == 0) so_holes = so_holes-1;
                            if (value == 1) so_sources = so_sources-1;
                            pointer_at_map->mappa[i][j].cell_type = value;
                        }
                        break;
                    case 1:
                        switch (col_pos) {
                            case 0:
                                if ((pointer_at_map->mappa[i-1][j].cell_type != 0) && 
                                        (pointer_at_map->mappa[i-1][j+1].cell_type != 0)) {
                                    do {
                                        value = rand() % (2-0+1) + 0;
                                    } while ((value == 0 && so_holes == 0) || 
                                            (value == 1 && so_sources == 0));
                                    if (value == 0) so_holes = so_holes-1;
                                    if (value == 1) so_sources = so_sources-1;
                                    pointer_at_map->mappa[i][j].cell_type = value;
                                } else {
                                    do {
                                        value = rand() % (2-1+1) + 1;
                                    } while (value == 1 && so_sources == 0);
                                    if (value == 1) so_sources = so_sources-1;
                                    pointer_at_map->mappa[i][j].cell_type = value;
                                }
                                break;
                            case 1:
                                if ((pointer_at_map->mappa[i][j-1].cell_type != 0) && 
                                        (pointer_at_map->mappa[i-1][j-1].cell_type != 0) &&
                                        (pointer_at_map->mappa[i-1][j].cell_type != 0) && 
                                        (pointer_at_map->mappa[i-1][j+1].cell_type != 0)) {
                                    do {
                                        value = rand() % (2-0+1) + 0;
                                    } while ((value == 0 && so_holes == 0) || 
                                            (value == 1 && so_sources == 0));
                                    if (value == 0) so_holes = so_holes-1;
                                    if (value == 1) so_sources = so_sources-1;
                                    pointer_at_map->mappa[i][j].cell_type = value;
                                } else {
                                    do {
                                        value = rand() % (2-1+1) + 1;
                                    } while (value == 1 && so_sources == 0);
                                    if (value == 1) so_sources = so_sources-1;
                                    pointer_at_map->mappa[i][j].cell_type = value;
                                }
                                break;
                            case 2:
                                if ((pointer_at_map->mappa[i][j-1].cell_type != 0) && 
                                        (pointer_at_map->mappa[i-1][j-1].cell_type != 0) &&
                                        (pointer_at_map->mappa[i-1][j].cell_type != 0)) {
                                    do {
                                        value = rand() % (2-0+1) + 0;
                                    } while ((value == 0 && so_holes == 0) || 
                                            (value == 1 && so_sources == 0));
                                    if (value == 0) so_holes = so_holes-1;
                                    if (value == 1) so_sources = so_sources-1;
                                    pointer_at_map->mappa[i][j].cell_type = value;
                                } else {
                                    do {
                                        value = rand() % (2-1+1) + 1;
                                    } while (value == 1 && so_sources == 0);
                                    if (value == 1) so_sources = so_sources-1;
                                    pointer_at_map->mappa[i][j].cell_type = value;
                                }
                                break;
                            default:
                                printf("Errore\n");
                                exit(EXIT_FAILURE);
                        }
                        break;
                    default:
                        printf("Errore\n");
                        exit(EXIT_FAILURE);
                }   
            }
        } 
    }
}
/* Nei casi in cui si odvesse verificare qualche anomalia viene restituito 1, 
 * ma per generare un errore cosa possiamo fare?
 */

/* Assegna ad ogni cella taxi_capacity*/
void random_taxi_capacity(map *pointer_at_map) {
    int i, j;
    for (i = 0; i < SO_HEIGHT; i++) {
        for (j = 0; j < SO_WIDTH; j++) {
            pointer_at_map->mappa[i][j].taxi_capacity = (rand() % (SO_CAP_MAX - SO_CAP_MIN + 1)) + SO_CAP_MIN;
        }
    }
}

/* Assegna ad ogni cella travel_time*/
void random_travel_time(map *pointer_at_map) {
    int i, j;
    for (i = 0; i < SO_HEIGHT; i++) {
        for (j = 0; j < SO_WIDTH; j++) {
            pointer_at_map->mappa[i][j].travel_time = (rand() % (SO_TIMENSEC_MAX - SO_TIMENSEC_MIN + 1)) + SO_TIMENSEC_MIN;
        }
    }
}

#endif

/* Da modificare: dovrà leggere i parametri da file e con rand
 * impostare i vari campi della struct. 
 * Se la mappa generata non è corretta si termina con errore
 * La codifica è: 0 hole, 1 SO_SOURCES, 2 no SO_SOURCES
 * Al momento usiamo una mappa 5x4 (quindi non importa quello che date
 * al programma in input che larghezza e altezza, ne faccio override nel
 * main) se volete lo schema della mappa che sto usando lo trovate nel 
 * documento condiviso
 */
void map_setup(map *pointer_at_map) {
    int i, j;
    srand(getpid());
    for (i = 0; i < SO_HEIGHT; i++) {
        for (j = 0; j < SO_WIDTH; j++) {
            pointer_at_map->mappa[i][j].cell_type = 2;
            pointer_at_map->mappa[i][j].active_taxis = 0;
        }
    }
#ifdef MAPPA_VALORI_CASUALI
    random_cell_type(pointer_at_map);
    random_taxi_capacity(pointer_at_map);
    random_travel_time(pointer_at_map);
#endif
#ifndef MAPPA_VALORI_CASUALI
    pointer_at_map->mappa[0][0].cell_type = 0;
    pointer_at_map->mappa[1][3].cell_type = 0;
    pointer_at_map->mappa[3][2].cell_type = 0;
    pointer_at_map->mappa[2][2].cell_type = 1;
#endif
    /* https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand */
}

/* Dovrebbe andare */
void map_print(map *pointer_at_map) {
    int i, j;
    pointer_at_map = shmat(shm_id, NULL, SHM_FLG);
    for (i = 0; i < SO_HEIGHT; i++) {
        for (j = 0; j < SO_WIDTH; j++) {
            printf ("%i ", pointer_at_map->mappa[i][j].cell_type);

#ifdef STAMPA_VALORI_CELLA
            printf ("%i", pointer_at_map->mappa[i][j].taxi_capacity);
            printf ("%i", pointer_at_map->mappa[i][j].active_taxis);
            printf ("%i", pointer_at_map->mappa[i][j].travel_time);
            printf ("%i", pointer_at_map->mappa[i][j].crossings);
#endif

        }
        printf("\n");
    }
}

void createIPC(map *pointer_at_map) {
    int i, j, counter = 0;
    /* Path per la ftok */
    char *path = "/tmp";
    /* Creo la memoria condivisa che contiene la mappa */
    shm_id = shmget (IPC_PRIVATE, sizeof(map), SHM_FLG);
    if (shm_id == -1) {
        perror("Non riesco a creare la memoria condivisa. Termino.");
        exit(EXIT_FAILURE);
    }
    /* Mi attacco come master alla mappa */
    pointer_at_map = shmat(shm_id, NULL, SHM_FLG);
    map_setup(pointer_at_map);
    /* Preparo gli argomenti per la execve */
    sprintf(m_id_str, "%d", shm_id); 
    args_a[1] = args_b[1] = m_id_str;
    /* Creo il semaforo per l'assegnazione delle celle 1 */
    sem_id = semget(SEM_KEY, 1, 0600 | IPC_CREAT); 
    /* Imposto il semaforo con valore 1 -MUTEX */
    semctl(sem_id, 0, SETVAL, 1);
    /* Creiamo le code di messaggi per le celle source */
    pointer_at_msgq = malloc(SO_SOURCES*sizeof(int));
    for (i = 0; i < SO_SOURCES; i ++) {
        pointer_at_msgq[i] = ftok(path, i);
        msgget(pointer_at_msgq[i], 0666 | IPC_CREAT | IPC_EXCL);
    }
    /* Assegna al campo della cella il valore della sua coda di messaggi*/
    for (i = 0; i < SO_HEIGHT; i ++){
        for (j = 0; j < SO_WIDTH; j++) {
            if (pointer_at_map->mappa[i][j].cell_type == 1) { 
                pointer_at_map->mappa[i][j].message_queue_key = pointer_at_msgq[counter];
                counter++;
            }
        }
    } 
}

void kill_all() {
    /* Completare. Dovrà terminare le risorse IPC che allocheremo. */
    int msqid, i;
    /* Marco per la deallocazione la memoria condivisa */
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
    for (i = 0; i < SO_SOURCES; i++) {
        msqid = msgget(pointer_at_msgq[i], 0600);
        /* Dealloco quella coda di messaggi */
        msgctl(msqid , IPC_RMID , NULL);
    }
}

/* Main */
int main () {

    int i, j, valore_fork_sources, valore_fork_taxi; 
    /* Lettura degli altri parametri specificati da file */
    reading_input_values();
#if 0
    printf("Stampo prima di inizializzare la mappa \n");
    map_print(pointer_at_map);
    printf("Stampo dopo l'inizializzazione della mappa \n");
    map_print(pointer_at_map);
#endif 
    /* Creo gli oggetti ipc */
    createIPC(pointer_at_map);
    /* Creo processi SO_SOURCES. Sistema gli argomenti */
    for (i = 0; i < SO_SOURCES; i++) {
        switch(valore_fork_sources = fork()) {
            case -1:
                printf("Errore nella fork. Esco.\n");
                kill_all();
                exit(EXIT_FAILURE);
                break;
            case 0:
                execve("Source", args_a, NULL);
                TEST_ERROR;
                break;
            default:
                /* Codice che voglio esegua il Master */
                break;
        }
    }

    /* Creo processi Taxi. Sistema gli argomenti */
    for (j = 0; j < SO_TAXI; j++) {
        switch(valore_fork_taxi = fork()) {
            case -1:
                printf("Errore nella fork. Esco.\n");
                kill_all();
                exit(EXIT_FAILURE);
                break;
            case 0:
                execve("Taxi", args_b, NULL);
                TEST_ERROR;
                break;
            default:
                /* Codice che voglio esegua il Master */
                break;
        }
    }

    /* Aspetto la terminazione dei figli */
    while(wait(NULL) != -1) {
        printf ("Ora tutti i figli sono terminati\n");
    }
    map_print(pointer_at_map);
    /* Stampo la coda di messaggi della cella 2.2 */
    /* Dealloca la memoria condivisa dove ho la mappa */
    kill_all();
    return 0;
}
