# Name: Jingjing 
# Email: 
# ID: 

CC = gcc
CFLAGS = -g -Wall -Wextra -lm -lmraa

default: build

build:
	$(CC) $(CFLAGS) lab4b.c -o lab4b

check:
	echo | ./lab4b --invalid > /dev/null 2>&1; \
	if [ $$? -ne 1 ]; then \
		echo "Error: invalid argument returns wrong exit code"; \
	else \
		echo "Invalid Argument Test: OK"; \
	fi

	./lab4b --period=3 --log=logTest --scale=C > /dev/null 2>&1; \
	if [ $$? -eq 1 ]; then \
		echo "Error: correct command returns wrong exit code"; \
	else \
		echo "Correct Command Test: OK"; \
	fi

clean: 
	rm -f lab4b *.tar.gz

dist: 
	tar -czf lab4b-304567417.tar.gz lab4b.c Makefile README
