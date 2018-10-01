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
int port = 0;
int log_command = 0;
int compress_command = 0;
int host = 0;
char* logFile;
char* hostName = "localhost";
int logFiledes=-1;
int socketFildes;
int BUFFER_SIZE = 256;
int stat;
int portNum;
z_stream toClient;
z_stream toServer;

void Error(char* errMsg)
{
    fprintf(stderr, "Error occurred when %s: %s\n", errMsg, strerror(errno));
    exit(1);
}

void restore_setting()
{    
	close(socketFildes);
    close(logFiledes);
    stat = tcsetattr(0, TCSANOW, &terminal_mode_ori);
    if (stat < 0)
        Error("restore setting");
}

void compressionEnd()
{
    inflateEnd(&toClient);
    deflateEnd(&toServer);
}

void compressionInit()
{
    toServer.zalloc = Z_NULL;
    toServer.zfree  = Z_NULL;
    toServer.opaque = Z_NULL;

    toClient.zalloc = Z_NULL;
    toClient.zfree  = Z_NULL;
    toClient.opaque = Z_NULL;

    int stat1 = inflateInit(&toClient);
    int stat2 = deflateInit(&toServer, Z_DEFAULT_COMPRESSION);
    if (stat1 != Z_OK || stat2 != Z_OK)
        Error("initializing the streams for compression");
    
    atexit(compressionEnd);
}

void echoInput(char* chars, int n, int outputFildes, int forward_server)
{
    char temp[2] = {'\r','\n'};
    int temp_size = sizeof(char)*2;

    char temp1[1]={'\n'};

    int i;
    for (i = 0; i < n; i++)
    {
        //CR: 0x0D, LF: 0x0A
        if (chars[i] == 0x0D || chars[i] == 0x0A)
        {
            if (forward_server == 1)
            {
                write(outputFildes, &temp1, sizeof(char));
            }
            else
            {
                write(outputFildes, &temp, temp_size);
            }
        }

        else
        {
            write(outputFildes, chars+i, sizeof(char));
        }
    }
}

void copy()
{
    int timeout = 0;

    // Open STREAMS device.
    struct pollfd fildes[2];
    fildes[0].fd = 0;
    fildes[1].fd = socketFildes;

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
                int read_stat = read(0, chars_client, BUFFER_SIZE);
                if (read_stat < 0)
                    Error("reading from client");
                else if (read_stat == 0)
                {
                    close(socketFildes);
                    exit(0);
                }

                //echo input from client to standard output
                echoInput(chars_client, read_stat, 1, 0);

                if (compress_command == 1)
                {
                    char chars_compr[BUFFER_SIZE];
                    toServer.avail_in = read_stat;
                    toServer.next_in = (unsigned char *) chars_client;
                    toServer.avail_out = BUFFER_SIZE;
                    toServer.next_out = (unsigned char *) chars_compr;

                    do
                    {
                        deflate(&toServer, Z_SYNC_FLUSH);
                    } while (toServer.avail_in > 0);

                    int chars_compr_size = BUFFER_SIZE - toServer.avail_out;

                    //forward input from client to server in compressed form
                    echoInput(chars_compr, chars_compr_size, socketFildes, 1);

                    if (log_command == 1)
                    {
                        stat = dprintf(logFiledes, "SENT %d bytes: ", chars_compr_size);
                        if (stat < 0)
                            Error("writing to the log file");
                        echoInput(chars_compr, chars_compr_size, logFiledes, 1);
                        write(logFiledes, "\n", 1);
                    }
                }
                else
                {
                    //forward input from client to server
                    echoInput(chars_client, read_stat, socketFildes, 1);

                    if (log_command == 1)
                    {
                        stat = dprintf(logFiledes, "SENT %d bytes: ", read_stat);
                        if (stat < 0)
                            Error("writing to the log file");
                        echoInput(chars_client, read_stat, logFiledes, 1);
                        write(logFiledes, "\n", 1);
                    }
                }

            }

            // Data may be read from server
            if (fildes[1].revents & POLLIN)
            {
                char chars_server[BUFFER_SIZE];
                int read_stat = read(fildes[1].fd, chars_server, BUFFER_SIZE);
                if (read_stat < 0)
                    Error("reading from server");
                else if (read_stat == 0)
                {
                    close(socketFildes);
                    exit(0);
                }

                if (compress_command == 1)
                {
                    char chars_compr[BUFFER_SIZE*4];
                    toClient.avail_in = read_stat;
                    toClient.next_in = (unsigned char *) chars_server;
                    toClient.avail_out = BUFFER_SIZE;
                    toClient.next_out = (unsigned char *) chars_compr;

                    do
                    {
                        inflate(&toClient, Z_SYNC_FLUSH);
                    } while (toClient.avail_in > 0);

                    int chars_compr_size = BUFFER_SIZE*4 - toServer.avail_out;

                    echoInput(chars_compr, chars_compr_size, 1, 0);
                }
                else
                {
                    echoInput(chars_server, read_stat, 1, 0);
                }

                if (log_command == 1)
                {
                    stat = dprintf(logFiledes, "RECEIVED %d bytes: ", read_stat);
                    if (stat < 0)
                        Error("writing to the log file");
                    echoInput(chars_server, read_stat, logFiledes, 1);
                    write(logFiledes, "\n", 1);
                }
            }

            //hangup or error
            if (fildes[1].revents & (POLLHUP | POLLERR))
                exit(0);
        }
    }
}


void restore(void) {
	close(socketFildes);
	close(logFiledes);
	tcsetattr(STDIN_FILENO, TCSANOW, &terminal_mode_ori);
}

int main(int argc, char *argv[])
{
    int opt, optIndex;

    static struct option long_options[] =
    {
        {"port",      required_argument, 0, 'p'},
        {"log",       required_argument, 0, 'l'},
        {"compress",  no_argument,       0, 'c'},
        {"host",      required_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "p:l:ch:", long_options, &optIndex)) != -1)
    {
        switch (opt)
        {
            case 'p':
                port = 1;
                portNum = atoi(optarg);
                break;
            case 'l':
                log_command = 1;
                logFiledes = creat(optarg, 0666);
                if (logFiledes < 0)
                    Error("creating the log file");
                break;
            case 'c':
                compress_command = 1;
                compressionInit();
                break;
            case 'h':
                host = 1;
                hostName = optarg;
                break;
            default:
                fprintf(stderr, "Usage: lab1a --port=portNum [--log=filename] [--compress] [--host=hostName]\n");
                exit(1);
        }
    }
    if (port == 0)
    {
        fprintf(stderr, "Error: port has to be specified through --port command line\n");
        exit(1);
    }


    //get the current terminal modes
    tcgetattr(0, &terminal_mode_ori);
    atexit(restore_setting);
    
    //make a copy with only the following changes
    struct termios terminal_mode_copy;
    tcgetattr(0, &terminal_mode_copy);
    
    terminal_mode_copy.c_iflag = ISTRIP;    /* only lower 7 bits    */
    terminal_mode_copy.c_oflag = 0;        /* no processing    */
    terminal_mode_copy.c_lflag = 0;        /* no processing    */
    
    int set = tcsetattr(0, TCSANOW, &terminal_mode_copy);
    if (set < 0)
        Error("setting up terminal mode");

    //setting the connection with server
    socketFildes = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFildes < 0)
        Error("opening the socket");

    struct hostent *server;
    server = gethostbyname(hostName);
    if (server == NULL)
        Error("getting the host");

    struct sockaddr_in ser_addr;
    memset((char *) &ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    memcpy((char*) &ser_addr.sin_addr.s_addr, (char*) server->h_addr, server->h_length);
    ser_addr.sin_port = htons(portNum);

    if (connect(socketFildes, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0)
        Error("connecting to the server");
    
    //start read and write processes
    copy();

    return 0;
}
