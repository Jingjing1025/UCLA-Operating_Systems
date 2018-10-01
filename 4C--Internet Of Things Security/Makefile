# Name: Jingjing 
# Email: 
# ID: 

CC = gcc
CFLAGS_TCP = -g -Wall -Wextra -lm -lmraa
CFLAGS_TLS = -g -Wall -Wextra -lm -lmraa -lssl -lcrypto

default: build

build:
	$(CC) $(CFLAGS_TCP) lab4c_tcp.c -o lab4c_tcp
	$(CC) $(CFLAGS_TLS) lab4c_tls.c -o lab4c_tls

clean: 
	rm -f lab4c_tcp lab4c_tls *.tar.gz

dist: 
	tar -czf lab4c-304567417.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README
