#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <strings.h>
#include "Function.h" 

void set_handler(int signum, void(*function)(int)) {

	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	sa.sa_handler = function;
	sa.sa_flags = 0;
	sigaction(signum, &sa, NULL);

}

/*
void ctrlc_handler(int signum) {

	printf("ane dio Ho ricevuto un control c \n");
    kill(1, SIGTERM);
    kill_all();
    kill(getpid(), SIGKILL);

}

void the_end_master(int signum) {
	int i;
	for (i = 0; i < SO_SOURCES; i++){
		kill(child_source[i], SIGTERM);
	}
	for (i = 0; i < SO_TAXI; i++){
		kill(child_taxi[i], SIGTERM);
	} 
}

void the_end_source(int signum) {
	kill(getpid(), SIGKILL);
}

void the_end_taxi(int signum) {
	kill(getpid(), SIGKILL);
}
*/
