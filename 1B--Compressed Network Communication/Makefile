# Name: Jingjing 
# Email: 
# ID: 

CC = gcc
CFLAGS = -g -Wall -Wextra -lz

default: lab1b-client lab1b-server

lab1b-client:
	$(CC) $(CFLAGS) lab1b-client.c -o lab1b-client

lab1b-server:
	$(CC) $(CFLAGS) lab1b-server.c -o lab1b-server

clean:
	rm -f lab1b-client lab1b-server *.o *.tar.gz

dist:
	tar -czf lab1b-304567417.tar.gz lab1b-client.c lab1b-server.c Makefile README
