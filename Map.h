#define SO_HEIGHT 4
#define SO_WIDTH 5

#define MAP_KEY 9876
#define SHM_FLG 0666
#define SEM_KEY 5432

#define TEST_ERROR    if (errno) {dprintf(STDERR_FILENO,        \
		"%s:%d: PID=%5d: Error %d (%s)\n", \
		__FILE__,         \
		__LINE__,         \
		getpid(),         \
		errno,            \
		strerror(errno));}

/* Struttura cella */
typedef struct 
{
	int cell_type;
	int taxi_capacity;
	int active_taxis;
	int travel_time;
	int crossings;
	int message_queue;
} cell;

/* http://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html */

typedef struct 
{
	cell mappa[SO_HEIGHT][SO_WIDTH];
} map;

/* Struct per la coda di messaggi */
