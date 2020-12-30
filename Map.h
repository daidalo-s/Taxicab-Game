#define TEST_ERROR    if (errno) {dprintf(STDERR_FILENO,        \
                      "%s:%d: PID=%5d: Error %d (%s)\n", \
                      __FILE__,         \
                      __LINE__,         \
                      getpid(),         \
                      errno,            \
                      strerror(errno));}
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
typedef struct 
{
	cell mappa[SO_HEIGHT][SO_WIDTH];
} map;
