#define SO_HEIGHT 10
#define SO_WIDTH 10
#define SHM_FLG 0600
#define SEM_FLG 0600
#define SOURCE_SEM_KEY 9876
#define TAXI_SEM_KEY 5432
#define INFINITY 9999
#define TAXI_SEM_ARRAY_DIM ((SO_WIDTH*SO_HEIGHT)-SO_HOLES)
#define MESSAGE_WIDTH 4
#define MAPPA_VALORI_CASUALI
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
	int message_queue_key;
	int reference_sem_number;
	int vertex_number;
} cell;

/* Spiega la memoria condivisa con esempi */
/* http://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html */

typedef struct 
{
	cell mappa[SO_HEIGHT][SO_WIDTH];
} map;

/* Struct per la coda di messaggi */
typedef struct message_queue 
{
	long mtype;     /* message type, must be > 0 */
	char message[MESSAGE_WIDTH];  /* message data */
} message_queue;

