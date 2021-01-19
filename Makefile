CFLAGS=-Wall -Werror -std=c89 -pedantic -D_GNU_SOURCE

OPTIONS =

all: Master Source Taxi 

Master: 
	gcc $(CFLAGS) Master.c -o Master

Source:
	gcc $(CFLAGS) Source.c -o Source

Taxi:
	gcc $(CFLAGS) Taxi.c -o Taxi

clean:
	rm -f Master Source Taxi


