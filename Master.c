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
#include <sys/mman.h>

#define SO_HEIGHT 4
#define SO_WIDTH 5
/* Struttura cella */

typedef struct 
{
	int cell_type;
	int taxi_capacity;
	int active_taxis;
	int travel_time;
	int crossings;
} cell;

/* http://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html */
/* Vettore dove tengo informazioni sui processi*/
typedef struct 
{
	cell mappa[SO_HEIGHT][SO_WIDTH];
} map;

#define TEST_ERROR    if (errno) {dprintf(STDERR_FILENO,		\
					  "%s:%d: PID=%5d: Error %d (%s)\n", \
					  __FILE__,			\
					  __LINE__,			\
					  getpid(),			\
					  errno,			\
					  strerror(errno));}


/****************** Prototipi ******************/
void kill_all();
void reading_input_values (); 
void random_cell_type(map *lamiamappadiocan);
void random_taxi_capacity(map *lamiamappadiocan);
void random_travel_time(map *lamiamappadiocan);
void map_creation(map *lamiamappadiocan);
void map_print(map *lamiamappadiocan);
void map_setup(map *lamiamappadiocan);
void free_map(map *lamiamappadiocan);

/* ---------------- Variabili globali ----------------- */
/* SO_WIDTH e SO_HEIGHT vengono letti dal main */
#if 1
map lamiamappadiocane;
map *lamiamappadiocan = &lamiamappadiocane;
#endif
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
/* Variabili per la creazione della mappa*/
key_t key = 9876; /* key da passare a shmget() */ 
int shmflg = 0666; /* shmflg da passare a shmget() */ 
int shmid; /* valore ritornato da shmget() */ 
int size; /* dimemsione da passare a shmget() */ 

/* ---------------- Lettura parametri da file ----------------- */
void reading_input_values () {

	char tmpstr1[16];
	char tmpstr2[16];
	char tempbuff[100];
	FILE *input = fopen("Parameters.txt", "r");
#if 0
	printf("Inizializzazione della simulazione \n");
	printf("Inserire la larghezza della mappa: \n");
	scanf("%i", &SO_WIDTH);
	printf("Inserire l'altezza della mappa: \n");
	scanf("%i", &SO_HEIGHT);
#endif
	if (input == NULL) {
		printf ("Errore, non riesco ad aprire il file \n");
		exit(-1); /* oppure return -1 */
	}

	while(!feof(input)) {

		if (fgets(tempbuff, sizeof(tempbuff),input)) {
			sscanf(tempbuff, "%15s = %15[^;];", tmpstr1, tmpstr2);

			if (strcmp(tmpstr1,"SO_HOLES")==0) {
				SO_HOLES = atoi(tmpstr2);
				if (SO_HOLES < 1) {
					printf("Errore, parametro SO_HOLES non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}       
			else if (strcmp(tmpstr1,"SO_TOP_CELLS")==0) {
				SO_TOP_CELLS = atoi(tmpstr2);
				if (SO_TOP_CELLS < 1) {
					printf("Errore, parametro SO_TOP_CELLS non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_SOURCES")==0) {
				SO_SOURCES = atoi(tmpstr2);
				if (SO_SOURCES < 1) {
					printf("Errore, parametro SO_SOURCES non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_CAP_MIN")==0) {
				SO_CAP_MIN = atoi(tmpstr2);
				if (SO_CAP_MIN < 0) {
					printf("Errore, parametro SO_CAP_MIN non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}  
			else if (strcmp(tmpstr1,"SO_CAP_MAX")==0) {
				SO_CAP_MAX = atoi(tmpstr2);
				if (SO_CAP_MAX < SO_CAP_MIN){
					printf("Errore, parametro SO_CAP_MAX non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_TAXI")==0) {
				SO_TAXI = atoi(tmpstr2);
				if (SO_TAXI < 0){
					printf("Errore, parametro SO_TAXI non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_TIMENSEC_MIN")==0) {
				SO_TIMENSEC_MIN = atoi(tmpstr2);
				if (SO_TIMENSEC_MIN < 0) {
					printf("Errore, parametro SO_TIMENSEC_MIN non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_TIMENSEC_MAX")==0) {
				SO_TIMENSEC_MAX = atoi(tmpstr2);
				if (SO_TIMENSEC_MAX < SO_TIMENSEC_MIN){
					printf("Errore, parametro SO_TIMENSEC_MAX non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_TIMEOUT")==0) {
				SO_TIMEOUT = atoi(tmpstr2);
				if (SO_TIMEOUT < 0) {
					printf("Errore, parametro SO_TIMEOUT non valido. Esco.\n");
					fclose(input);
					exit(1);
				}
			}
			else if (strcmp(tmpstr1,"SO_DURATION")==0) {
				SO_DURATION = atoi(tmpstr2);
				if (SO_DURATION < SO_TIMEOUT) {
					printf("Errore, parametro SO_TIMEOUT non valido. Esco.\n");
					fclose(input);
					exit(1);
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
void random_cell_type(map *lamiamappadiocan) {
	/* Variabili locali utilizzate:
	 * - value assume il valore della codifica (0 hole, 1 no SO_SOURCES, 2 cella libera) 
	 *   da assegnare alla cella[i][j]; i e j assumono il valore degli indici della cella di riferimento;
	 * - row_pos (row position) e col_pot (column position) sono variabili utilizzate 
	 *   per capire in seguito quale case eseguire all'interno degli switch;
	 * - so_holes e so_sources assumono i valori delle variabili SO_HOLES ed SO_SOURCES,
	 *   dato che quest'ultime sono variabili globali e' meglio non modificarne il valore
	 *   ma bensi' utilizzarne una copia all'interno della funzione
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
				map[i][j].cell_type = value;
			} else {    
				switch (row_pos) {
					case 0:
						if (map[i][j-1].cell_type == 0) { 
							do {
								value = rand() % (2-1+1) + 1;
							} while (value == 1 && so_sources == 0);
							if (value == 1) so_sources = so_sources-1;
							map[i][j].cell_type = value;
						} else {
							do {
								value = rand() % (2-0+1) + 0;
							} while ((value == 0 && so_holes == 0) || (value == 1 && so_sources == 0));
							if (value == 0) so_holes = so_holes-1;
							if (value == 1) so_sources = so_sources-1;
							map[i][j].cell_type = value;
						}
						break;
					case 1:
						switch (col_pos) {
							case 0:
								if ((map[i-1][j].cell_type != 0) && (map[i-1][j+1].cell_type != 0)) {
									do {
										value = rand() % (2-0+1) + 0;
									} while ((value == 0 && so_holes == 0) || (value == 1 && so_sources == 0));
									if (value == 0) so_holes = so_holes-1;
									if (value == 1) so_sources = so_sources-1;
									map[i][j].cell_type = value;
								} else {
									do {
										value = rand() % (2-1+1) + 1;
									} while (value == 1 && so_sources == 0);
									if (value == 1) so_sources = so_sources-1;
									map[i][j].cell_type = value;
								}
								break;
							case 1:
								if ((map[i][j-1].cell_type != 0) && (map[i-1][j-1].cell_type != 0) &&
										(map[i-1][j].cell_type != 0) && (map[i-1][j+1].cell_type != 0)) {
									do {
										value = rand() % (2-0+1) + 0;
									} while ((value == 0 && so_holes == 0) || (value == 1 && so_sources == 0));
									if (value == 0) so_holes = so_holes-1;
									if (value == 1) so_sources = so_sources-1;
									map[i][j].cell_type = value;
								} else {
									do {
										value = rand() % (2-1+1) + 1;
									} while (value == 1 && so_sources == 0);
									if (value == 1) so_sources = so_sources-1;
									map[i][j].cell_type = value;
								}
								break;
							case 2:
								if ((map[i][j-1].cell_type != 0) && (map[i-1][j-1].cell_type != 0) &&
										(map[i-1][j].cell_type != 0)) {
									do {
										value = rand() % (2-0+1) + 0;
									} while ((value == 0 && so_holes == 0) || (value == 1 && so_sources == 0));
									if (value == 0) so_holes = so_holes-1;
									if (value == 1) so_sources = so_sources-1;
									map[i][j].cell_type = value;
								} else {
									do {
										value = rand() % (2-1+1) + 1;
									} while (value == 1 && so_sources == 0);
									if (value == 1) so_sources = so_sources-1;
									map[i][j].cell_type = value;
								}
								break;
							default:
								printf("Errore\n");
								exit(-1);
						}
						break;
					default:
						printf("Errore\n");
						exit(-1);
				}   
			}
		} 
	}
}

/* Nei casi in cui si odvesse verificare qualche anomalia viene restituito 1, 
 * ma per generare un errore cosa possiamo fare?
 */

/* Assegna ad ogni cella taxi_capacity*/
void random_taxi_capacity(map *lamiamappadiocan) {
	int i, j;
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			lamiamappadiocan->mappa[i][j].taxi_capacity = (rand() % (SO_CAP_MAX - SO_CAP_MIN + 1)) + SO_CAP_MIN;
		}
	}
}

/* Assegna ad ogni cella travel_time*/
void random_travel_time(map *lamiamappadiocan) {
	int i, j;
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			lamiamappadiocan->mappa[i][j].travel_time = (rand() % (SO_TIMENSEC_MAX - SO_TIMENSEC_MIN + 1)) + SO_TIMENSEC_MIN;
		}
	}
}

#endif

/* Da modificare: dovrà leggere i parametri da file e con rand
 * impostare i vari campi della struct. 
 * Se la mappa generata non è corretta si termina con errore
 * La codifica è: 0 hole, 1 SO_SOURCE, 2 no SO_SORUCE
 * Al momento usiamo una mappa 5x4 (quindi non importa quello che date
 * al programma in input che larghezza e altezza, ne faccio override nel
 * main) se volete lo schema della mappa che sto usando lo trovate nel 
 * documento condiviso
 */
void map_setup(map *lamiamappadiocan) {
	int i, j;
	srand(getpid());
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			lamiamappadiocan->mappa[i][j].cell_type = 2;
			lamiamappadiocan->mappa[i][j].active_taxis = 0;
		}
	}
#ifdef MAPPA_VALORI_CASUALI
	random_cell_type(map);
	random_taxi_capacity(map);
	random_travel_time(map);
#endif
#ifndef MAPPA_VALORI_CASUALI
	lamiamappadiocan->mappa[0][0].cell_type = 0;
	lamiamappadiocan->mappa[1][3].cell_type = 0;
	lamiamappadiocan->mappa[3][2].cell_type = 0;
	lamiamappadiocan->mappa[2][2].cell_type = 1;
#endif
	/* https://stackoverflow.com/questions/1202687/how-do-i-get-a-specific-range-of-numbers-from-rand */
}

/* Dovrebbe andare */
void map_print(map *lamiamappadiocan) {
	int i, j;
#if 0
	printf("La larghezza della mappa è: %i\n", SO_WIDTH);
#endif
	for (i = 0; i < SO_HEIGHT; i++) {
		for (j = 0; j < SO_WIDTH; j++) {
			printf ("%i ", lamiamappadiocan->mappa[i][j].cell_type);

#ifdef STAMPA_VALORI_CELLA
			printf ("%i", lamiamappadiocan->mappa[i][j].taxi_capacity);
			printf ("%i", lamiamappadiocan->mappa[i][j].active_taxis);
			printf ("%i", lamiamappadiocan->mappa[i][j].travel_time);
			printf ("%i", lamiamappadiocan->mappa[i][j].crossings);
#endif

		}
		printf("\n");
	}
}

/* Facciamo che al momento pulisce la memoria condivisa */
void kill_all(map *lamiamappadiocan) {
	/* Dobbiamo bloccare i segnali ke ankora nn abbiamo okkio*/

	/* Per terminare i processi scorriamo la matrice e appena ne
	   troviamo uno zap */
	shmctl(shmid,IPC_RMID,NULL); 

}

#if 0
int size = sizeof(map);
	if ((shmid = shmget (IPC_PRIVATE, size, shmflg)) == -1) {
		perror("shmget: shmget failed"); 
		exit(1); 
	} else {
		 shmat(shmid, NULL, shmflg);
		/* Non controllo se non riesco ad attaccarmi */
	}
#endif

/* Main */
int main () {
	int i, j, valore_fork_sources, valore_fork_taxi;
	char * args_a[] = {"Source", NULL, NULL};
	char * args_b[] = {"Taxi", NULL, NULL};
	char m_id_str[13]; 
	/* Lettura degli altri parametri specificati da file */
	reading_input_values();
	/* Creazione e inizializzazione mappa */
	map_setup(lamiamappadiocan);
	if ((shmid = shmget (IPC_PRIVATE, sizeof(map), shmflg)) == -1) {
		perror("Bastarda la madonna non vado a dormire "); 
		exit(1); 
	} else {
		 shmat(shmid, NULL, shmflg);
		/* Non controllo se non riesco ad attaccarmi */
	}
	sprintf(m_id_str, "%d", shmid);
	args_b[1] = m_id_str;
	args_a[1] = m_id_str;
	/* Creo processi SO_SOURCES. Sto passando argomenti fittizi.*/
	for (i = 0; i < SO_SOURCES; i++) {
		switch(valore_fork_sources = fork()) {
			case -1:
				printf("Errore nella fork. Esco.\n");
				/*Il metodo al momento fa solo una free -SBAGLIATO-*/
				kill_all(lamiamappadiocan);
				break;
			case 0:
				printf("Qua arrivo dio stupido \n");
				execve("Source", args_a, NULL);
				TEST_ERROR;
				break;
			default:
				printf("Sono il main in SO_SOURCES \n");
				break;
		}
	}

	/* Creo processi Taxi. Sto passando argomenti fittizi. */
	for (j = 0; j < SO_TAXI; j++) {
		switch(valore_fork_taxi = fork()) {
			case -1:
				printf("Errore nella fork. Esco.\n");
				/*Il metodo al momento fa solo una free -SBAGLIATO-*/
				kill_all(lamiamappadiocan);
				break;
			case 0:
				if (execve("Taxi", args_b, NULL) == -1) {
					printf("Almeno qualcosa non funziona \n");
				};
				break;
			default:
				printf("Sono il main in SO_TAXI\n");
				break;
		}
	}
	/* Stampa tante volte quanti sono i processi che aspetta*/
	while(wait(NULL) != -1) {
		printf ("Ora tutti i figli sono terminati\n");
	}
	map_print(lamiamappadiocan);
	kill_all(lamiamappadiocan);
	return 0;
}
