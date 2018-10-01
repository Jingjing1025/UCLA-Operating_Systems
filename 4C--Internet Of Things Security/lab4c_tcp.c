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
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <mraa.h>
#include <aio.h>


int stat;
int interval = 1;
int host = 0;
int log_command = 1;
int report = 1;
FILE* logFiledes = NULL;
int portNum = -1;
int socketFildes;
char opt_scale = NULL;
char* logFile;
char* idNum = NULL;
char* hostName = NULL;
int BUFFER_SIZE = 1024;

mraa_aio_context tempSensor;

struct timeval m_time;
time_t n_time = 0;
struct tm *l_time;

void Error(char* errMsg)
{
    fprintf(stderr, "Error occurred when %s: %s\n", errMsg, strerror(errno));
    exit(2);
}

void shut_down()
{
    l_time = localtime( &m_time.tv_sec );

    dprintf(socketFildes, "%02d:%02d:%02d SHUTDOWN\n", l_time->tm_hour, l_time->tm_min, l_time->tm_sec);
    
    if(log_command == 1)
    {
        fprintf( logFiledes, "%02d:%02d:%02d SHUTDOWN\n", l_time->tm_hour, l_time->tm_min, l_time->tm_sec );
    }

    mraa_aio_close(tempSensor);

    exit(0);
}

void period_and_log(char* temp)
{
    int i;
    char p[8] = "PERIOD=";
    char l[4] = "LOG";
    int valid_p = 0;
    int valid_l = 0;

    if (strlen(temp) > strlen(p))
    {
        valid_p = 1;

        for (i = 0; i < 7; i++)
        {
            if (temp[i] != p[i])
            {
                valid_p = 0;
                break;
            }
        }
    }

    if (valid_p == 1)
    {
        int len = strlen(temp)-7;
        char digit[10];
        for (i = 0; i < len; i++)
        {
            if (!isdigit(temp[i+7]))
            {
                valid_p = 0;
                fprintf(stderr, "Invalid Command\n");
                return;
            }
            digit[i] = temp[i+7];
        }
        interval = atoi(digit);
    }

    if (strlen(temp) > strlen(l))
    {
        valid_l = 1;

        for (i = 0; i < 3; i++)
        {
            if (temp[i] != l[i])
            {
                valid_l = 0;
                break;
            }
        }
    }

    if (valid_l == 1 || valid_p == 1)
    {
        fprintf(logFiledes, "%s\n", temp);
    }
    else
    {
        fprintf(stderr, "Invalid Commands\n");
        return;
    }
}


int main(int argc, char *argv[])
{
    int opt, optIndex;

    static struct option long_options[] =
    {
        {"period", required_argument, NULL, 'p'},
        {"scale",  required_argument, NULL, 's'},
        {"log",    required_argument, NULL, 'l'},
        {"id",     required_argument, NULL, 'i'},
        {"host",   required_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "p:s:l:i:h:", long_options, &optIndex)) != -1)
    {
        switch (opt)
        {
            case 'p':
                interval = atoi(optarg);
                break;
            case 's':
                if (*optarg == 'C' || *optarg == 'F')
                {
                    opt_scale = *optarg;
                    break;
                }
                else
                {
                    fprintf(stderr, "Usage: lab4b [--period=interval] [--scale={CF}] [--log=fileName] [--id=idNum] [--host=hostName] \n");
                    exit(1);
                }
            case 'l':
                log_command = 1;
                logFiledes = fopen(optarg, "w+");
                if (logFiledes == NULL)
                    Error("openning the log file");
                break;
            case 'i':
                if (strlen(optarg) != 9)
                    Error("extracting id number");
                idNum = optarg;
                break;
            case 'h':
                host = 1;
                hostName = optarg;
                break;
            default:
                fprintf(stderr, "Usage: lab4b [--period=interval] [--scale={CF}] [--log=fileName] [--id=idNum] [--host=hostName] \n");
                exit(1);
        }
    }

    if (log_command == 0 || host == 0)
    {
        fprintf(stderr, "Usage: lab4b [--period=interval] [--scale={CF}] [--log=fileName] [--id=idNum] [--host=hostName] \n");
        exit(1);
    }

    portNum = atoi(argv[optind]);
    if (portNum == -1 || portNum == 0)
        Error("extracting the port number");

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

    dprintf(socketFildes, "ID=%s\n", idNum);
    fprintf(logFiledes, "ID=%s\n", idNum);

    tempSensor = mraa_aio_init(1);
    if (tempSensor == NULL)
    {
        fprintf(stderr, "Failed to initialize tempSensor\n");
        mraa_deinit();
        exit(1);
    }


    int timeout = 0;

    struct pollfd fildes;
    fildes.fd = socketFildes;
    fildes.events = POLLIN | POLLHUP | POLLERR;

    while (1)
    {
        stat = poll(&fildes, 1, timeout);
        if (stat < 0)
            Error("using poll system");
        else
        {
            if (report == 1)
            {
                gettimeofday(&m_time, 0);

                if (m_time.tv_sec - n_time >= 0)
                {
                    stat = mraa_aio_read(tempSensor);
                    if (stat < 0)
                    {
                        fprintf(stderr, "Error occurred when reading tempSensor status\n");
                        mraa_aio_close(tempSensor);
                        exit(1);
                    }

                    double R = 1023.0/((double)stat) - 1.0;
                    R = R * 100000.0;
                    double temp_C = 1.0/(log(R/100000.0)/4275 + 1/298.15) - 273.15;
                    double temp_F = temp_C * 9/5 + 32;
                    double temp;

                    if (opt_scale == 'F')
                        temp = temp_F;
                    else if (opt_scale == 'C')
                        temp = temp_C;

                    l_time = localtime(&m_time.tv_sec);

                    dprintf(socketFildes, "%02d:%02d:%02d %0.1f\n", l_time->tm_hour, l_time->tm_min, l_time->tm_sec, temp);
                    
                    fprintf(logFiledes, "%02d:%02d:%02d %0.1f\n", l_time->tm_hour, l_time->tm_min, l_time->tm_sec, temp);

                    n_time = m_time.tv_sec + interval;
                }


            }

            if( fildes.revents & POLLIN )
            {
                char chars[BUFFER_SIZE];
                int size = BUFFER_SIZE * sizeof(char);
                memset(chars, 0, size);
                stat = read(socketFildes, chars, size);
                if (stat < 0)
                    Error("reading from socket");
                else
                {
                    int i;
                    int count = 0;
                    char temp[100]="";

                    for (i = 0; i < stat; i++)
                    {
                        if (chars[i] != '\n')
                        {
                            char c = chars[i];
                            temp[count] = c;
                            count += 1;
                        }

                        else
                        {
                            temp[count] = '\0';

                            if (strcmp(temp, "SCALE=F") == 0 || strcmp(temp, "SCALE=C") == 0 )
                            {
                                opt_scale = temp[6];
                                fprintf(logFiledes, "%s\n", temp);
                            }

                            else if (strcmp(temp, "START") == 0)
                            {
                                fprintf(logFiledes, "%s\n", temp);
                            }
                            else if (strcmp(temp, "STOP") == 0)
                            {
                                report = 0;
                                fprintf(logFiledes, "%s\n", temp);
                            }
                            else if (strcmp(temp, "OFF") == 0)
                            {
                                fprintf(logFiledes, "%s\n", temp);
                                shut_down();
                            }
                            else
                            {
                                period_and_log(temp);
                            }

                            strcpy(temp, "");
                            count = 0;
                        }

                    }

                }
            }

        }
    }

    mraa_aio_close(tempSensor);
    close(socketFildes);

}
