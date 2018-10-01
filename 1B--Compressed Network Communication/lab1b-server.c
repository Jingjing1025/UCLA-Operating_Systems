// Name: Jingjing 
// Email: 
// ID:

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "zlib.h"

struct termios terminal_mode_ori;
int fromTerminal[2];
int toTerminal[2];
int BUFFER_SIZE = 256;
int port = 0;
int compress_command = 0;
int stat;
int child_pid;
int portNum;
int socketFildes;
int socketFildes_cli;
int Dflag = 0;
z_stream toClient;
z_stream toServer;

void Error(char* errMsg)
{
    fprintf(stderr, "Error occurred when %s: %s\n", errMsg, strerror(errno));
    exit(1);
}

void complete_status()
{
    int status;
    stat = waitpid(child_pid, &status, 0);
    if (stat == -1)
        Error("executing waitpid");
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
}

void close_with_status()
{
    close(socketFildes);
    close(socketFildes_cli);
    close(toTerminal[0]);
    close(fromTerminal[1]);
    complete_status();
}

void signal_handler(int n)
{
    if (n == SIGINT)
        kill(child_pid, SIGINT);
    if (n == SIGPIPE)
    {
        complete_status();
        exit(0);
    }
}

void echoInput(char* chars, int n, int outputFildes)
{
    int i;
    for (i = 0; i < n; i++)
    {
        //EOF: 0x04
        if (chars[i] == 0x04)
        {
            Dflag = 1;
        }
        
        //EOT: 0x03
        else if (chars[i] == 0x03)
        {
            kill(child_pid, SIGINT);
        }
        else
        {
           write(outputFildes, chars+i, sizeof(char));
        }
    }
}

void echoInput_comp(char* chars, int n, int outputFildes)
{
    int a = 0;
    int b = 0;
    int i = 0;
    for (i = 0; i < n; i++)
    {
        if (chars[i] == 0x03)
        {
            kill(child_pid, SIGINT);
        }
        else if (chars[i] == 0x04)
        {
            Dflag = 1;
        }
        else if (chars[i] == 0x0D || chars[i] == 0x0A)
        {
            char chars_compr[BUFFER_SIZE];
            toClient.avail_in = a;
            toClient.next_in = (unsigned char *) (chars + b);
            toClient.avail_out = BUFFER_SIZE;
            toClient.next_out = (unsigned char *) chars_compr;
            a++;
            do
            {
                deflate(&toClient, Z_SYNC_FLUSH);
            } while (toClient.avail_in > 0);
            
            int chars_compr_size = BUFFER_SIZE - toClient.avail_out;
            write(outputFildes, chars_compr, chars_compr_size);
            
            b += a;
            a = -1;
        }
        a++;
    }
    write(outputFildes, chars+b, a);
}


void compressionEnd()
{
    inflateEnd(&toServer);
    deflateEnd(&toClient);
}

void compressionInit()
{
    toServer. zalloc = Z_NULL;
    toServer. zfree  = Z_NULL;
    toServer. opaque = Z_NULL;
    
    toClient. zalloc = Z_NULL;
    toClient. zfree  = Z_NULL;
    toClient. opaque = Z_NULL;
    
    int stat1 = inflateInit(&toServer);
    int stat2 = deflateInit(&toClient, Z_DEFAULT_COMPRESSION);
    if (stat1 != Z_OK || stat2 != Z_OK)
        Error("initializing the streams for compression");
    
    atexit(compressionEnd);
}

void copy_shell()
{
    int timeout = 0;
    Dflag = 0;
    
    // Open STREAMS device.
    struct pollfd fildes[2];
    fildes[0].fd = socketFildes_cli;
    fildes[1].fd = toTerminal[0];
    
    fildes[0].events = POLLIN | POLLHUP | POLLERR;
    fildes[1].events = POLLIN | POLLHUP | POLLERR;
    
    while (1)
    {
        stat = poll(fildes, 2, timeout);
        if (stat < 0)
            Error("using poll system");
        else if (stat == 0)
            continue;
        else
        {
            // Data may be read from client
            if (fildes[0].revents & POLLIN)
            {
                char chars_client[BUFFER_SIZE];
                int read_stat = read(fildes[0].fd, chars_client, BUFFER_SIZE);
                if (read_stat < 0)
                    Error("reading from client");
                else if (read_stat == 0)
                {
                    complete_status();
                    exit(0);
                }
                
                if (compress_command == 1)
                {
                    char chars_compr[BUFFER_SIZE*4];
                    toServer.avail_in = read_stat;
                    toServer.next_in = (unsigned char *) chars_client;
                    toServer.avail_out = BUFFER_SIZE*4;
                    toServer.next_out = (unsigned char *) chars_compr;
                    
                    do
                    {
                        inflate(&toServer, Z_SYNC_FLUSH);
                    } while (toServer.avail_in > 0);
                    
                    int chars_compr_size = BUFFER_SIZE*4 - toServer.avail_out;
                    
                    //forward input from client to server in compressed form
                    echoInput(chars_compr, chars_compr_size, fromTerminal[1]);
                }
                else
                {
                    //forward input from client to server
                    echoInput(chars_client, read_stat, fromTerminal[1]);
                }
                
            }
            
            // Data may be read from shell
            if (fildes[1].revents & POLLIN)
            {
                char chars_server[BUFFER_SIZE];
                int read_stat = read(fildes[1].fd, chars_server, BUFFER_SIZE);
                if (read_stat < 0)
                    Error("reading from server");
                else if (read_stat == 0)
                {
                    complete_status();
                    exit(0);
                }
                
                if (compress_command == 1)
                {
                    echoInput_comp(chars_server, read_stat, socketFildes_cli);
                }
                else
                {
                    echoInput(chars_server, read_stat, socketFildes_cli);
                }
            }
            //hangup or error
            if (fildes[1].revents & (POLLHUP | POLLERR) || Dflag)
            {
                // kill(child_pid, SIGINT);
                close(fildes[1].fd);
                complete_status();
                break;
            }
        }
    }
}

void child_process()
{
    //close unnecessary pipe directions
    close(fromTerminal[1]);
    close(toTerminal[0]);
    
    dup2(fromTerminal[0], 0);
    dup2(toTerminal[1], 1);
    dup2(toTerminal[1], 2);
    
    //close used pipes after usage
    close(fromTerminal[0]);
    close(toTerminal[1]);
    
    char* execvp_arg[2]={"/bin/bash", NULL};
    stat = execvp("/bin/bash",execvp_arg);
    if (stat == -1)
    {
        Error("executing child process");
    }
    
}

void parent_process()
{
    close(fromTerminal[0]);
    close(toTerminal[1]);
    
    copy_shell();
}

int main(int argc, char *argv[])
{
    int opt, optIndex;
    
    static struct option long_options[] =
    {
        {"port",      required_argument, 0, 'p'},
        {"compress",  no_argument,       0, 'c'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "p:c", long_options, &optIndex)) != -1)
    {
        switch (opt)
        {
            case 'p':
                port = 1;
                portNum = atoi(optarg);
                break;
            case 'c':
                compress_command = 1;
                compressionInit();
                break;
            default:
                fprintf(stderr, "Usage: lab1a --port=portNum [--compress]\n");
                exit(1);
        }
    }
    if (port == 0)
    {
        fprintf(stderr, "Error: port has to be specified through --port command line\n");
        exit(1);
    }
    
    //Setting the connection with the client
    socketFildes = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFildes < 0)
        Error("opening the socket");
    
    struct sockaddr_in ser_addr, cli_addr;
    memset((char *) &ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = INADDR_ANY;
    ser_addr.sin_port = htons(portNum);
    
    if (bind(socketFildes, (struct sockaddr *) &ser_addr, sizeof(ser_addr)) < 0)
        Error("binding the socket");
    if (listen(socketFildes, 8) < 0)
        Error("telling the socket to accept new connection");
    unsigned int cli = sizeof(cli_addr);
    socketFildes_cli = accept(socketFildes, (struct sockaddr *) &cli_addr, &cli);
    if (socketFildes_cli < 0)
        Error("accepting the connection");
    
    //Setting up pipes and fork the child process
    signal(SIGPIPE, signal_handler);
    //build two pipes for both directions
    int stat1 = pipe(fromTerminal);
    int stat2 = pipe(toTerminal);
    if (stat1 == -1 || stat2 == -1)
        Error("building the pipe");
    
    //use fork to generate a child process
    child_pid = fork();
    if (child_pid == -1)
        Error("creating new process");
    else if (child_pid == 0)
        child_process();
    else
        parent_process();
    
    return 0;
}
