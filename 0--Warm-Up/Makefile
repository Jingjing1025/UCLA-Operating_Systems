# Name: Jingjing 
# Email: 
# ID: 

.SILENT:

CC = gcc
CFLAGS = -g -Wall -Wextra

default: lab0.c
	$(CC) $(CFLAGS) lab0.c -o lab0

check: default check_success check_segfault check_input check_output check_argument

check_success:
	echo "smoke-test" > input_test.txt
	./lab0 --input input_test.txt --output output_test.txt &> /dev/null;\
	if [ $$? -ne 0 ]; then \
		echo "Success Check Failed: successful case did not result in exit code 0"; \
	else \
		echo "Success Check Passed"; \
	fi

check_segfault:
	./lab0 --segfault --catch &> /dev/null;\
	if [ $$? -ne 4 ]; then \
		echo "Segfault Check Failed: segmentation fault caught did not result in exit code 4";\
	else \
		echo "Segfault Check Passed"; \
	fi

check_input:
	./lab0 --input nonexist_file.txt --output output_test.txt &> /dev/null;\
	if [ $$? -ne 2 ]; then \
		echo "Input Check Failed: failure opening input file did not result in exit code 2";\
	else \
		echo "Input Check Passed"; \
	fi

check_output:
	touch output.txt      
	chmod -w output.txt
	./lab0 --output output.txt &> /dev/null;\
	if [ $$? -ne 3 ]; then \
		echo "Output Check Failed: failure creating output file did not result in exit code 3";\
	else \
		echo "Output Check Passed"; \
	fi

check_argument:
	./lab0 --wrong_argument &> /dev/null;\
	if [ $$? -ne 1 ]; then \
		echo "Argument Check Failed: wrong argument did not result in exit code 1";\
	else \
		echo "Argument Check Passed"; \
	fi

clean:
	rm -f lab0 *txt *.o *.tar.gz

dist:
	tar -czf lab0-304567417.tar.gz lab0.c Makefile README backtrace.png breakpoint.png

