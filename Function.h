#define SO_HEIGHT 10
#define SO_WIDTH 20
#define SHM_FLG 0666
#define SEM_FLG 0600
#define SOURCE_SEM_KEY 9876
#define TAXI_SEM_KEY 5432
#define START_SEM_KEY 1098
#define INFO_SEM_KEY 7654
#define INFINITY 9999
#define TAXI_SEM_ARRAY_DIM ((SO_WIDTH*SO_HEIGHT)-SO_HOLES)
#define MESSAGE_WIDTH 7

/* Struttura cella */
typedef struct {
	int cell_type;
	int taxi_capacity;
	int active_taxis;
	int travel_time;
	int crossings;
	int message_queue_key;
	int reference_sem_number;
	int vertex_number;
	int completed_trip;
	int aborted_trip;
} cell;

typedef struct {
	int pid;
	int service_time; /* Tempo */
	int served_clients; /* Clienti raccolti */
	int longest_trip; /* Numero celle */
} taxi_info;


typedef struct {
	cell mappa[SO_HEIGHT][SO_WIDTH];
} map;

/* Struct per la coda di messaggi */
typedef struct message_queue {
	long mtype;     /* message type, must be > 0 */
	char message[MESSAGE_WIDTH];  /* message data */
} message_queue;

void set_handler(int signum, void(*function)(int));
