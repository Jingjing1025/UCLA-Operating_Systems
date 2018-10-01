# Name: Jingjing 
# Email: 
# ID: 

CC = gcc
CFLAGS = -g -Wall -Wextra 

default: build

build:
	$(CC) $(CFLAGS) lab3a.c -o lab3a

clean: 
	rm -f lab3a *.tar.gz

dist: 
	tar -czf lab3a-304567417.tar.gz lab3a.c ext2_fs.h Makefile README
