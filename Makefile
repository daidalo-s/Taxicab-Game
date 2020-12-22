CFLAGS=-Wall -Werror -std=c89 -pedantic -D_GNU_SOURCE

OPTIONS =

all: Master

Master: 
	gcc $(CFLAGS) Master.c -o Master

clean:
	rm -f Master
	