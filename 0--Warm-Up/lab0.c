// Name: Jingjing
// Email: 
// ID: 

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

void copy()
{
	char currChar;
	ssize_t stat = read(0, &currChar, sizeof(char));
	while (stat > 0)
	{
		write(1, &currChar, sizeof(char));
		stat = read(0, &currChar, sizeof(char));
	}
}

void newInput(int inputFd, char* input_file)
{
	if (inputFd >= 0)
	{
		close(0);
		dup(inputFd);
		close(inputFd);
	}
	else
	{
		fprintf(stderr, "Error opening file %s: %s\n", input_file, strerror(errno));
		exit(2);
	}

}

void newOutput(int outputFd, char* output_file)
{
	if (outputFd >= 0)
	{
		close(1);
		dup(outputFd);
		close(outputFd);
	}
	else
	{
		fprintf(stderr, "Error creating the file %s: %s\n", output_file, strerror(errno));
		exit(3);
	}
}

void signal_handler(int n)
{
	if (n == SIGSEGV)
	{
		fprintf(stderr, "Error: Segmentation fault, %s\n", strerror(errno));
		exit(4);
	}
}

int main(int argc, char *argv[])
{
	int opt, optIndex;
	int inputFd, outputFd;
	char* input_file = NULL;
	char* output_file = NULL;
	int segFault = 0;
	int cat = 0;

	static struct option long_options[] =
	{
		{"input",    required_argument, 0, 'i'},
		{"output",   required_argument, 0, 'o'},
		{"segfault", no_argument,       0, 's'},
		{"catch",    no_argument,       0, 'c'},
		{0, 0, 0, 0}
	};
  	
  	while ((opt = getopt_long(argc, argv, "i:o:sc", long_options, &optIndex)) != -1) 
  	{
  		switch (opt)
  		{
  			case 'i':
  				input_file = optarg;
  				break;  				
  			case 'o':
  				output_file = optarg;
  				break;
  			case 's':
  				segFault = 1;
  				break;
  			case 'c':
  				cat = 1;
  				break;
  			default:
  				fprintf(stderr, "Usage: lab0 --input input_file_name --output output_file_name [--segfault] [--catch]\n");
  				exit(1);
  		}
  	}

  	if (cat == 1)
  	{
  		signal(SIGSEGV, signal_handler);
  	}
  	
  	if (segFault == 1)
  	{
  		char* ptr = NULL;
  		*ptr = 'A';
  	}

  	if (input_file != NULL)
  	{
  		inputFd = open(input_file, O_RDONLY);
  		newInput(inputFd, input_file);
  	}

  	if (output_file != NULL)
  	{
  		outputFd = creat(output_file, S_IRWXU);
  		newOutput(outputFd, output_file);
  	}

  	copy();

  	exit(0);

 }