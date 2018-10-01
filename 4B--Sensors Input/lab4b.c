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
#include <sys/time.h>
#include <mraa.h>
#include <aio.h>

int stat;
int log_command = 0;
int interval = 1;
int report = 1;
char opt_scale = NULL;
FILE* logFiledes = NULL;
int BUFFER_SIZE = 1024;

mraa_aio_context tempSensor;
mraa_gpio_context button;

struct timeval m_time;
time_t n_time = 0;
struct tm *l_time;

void Error(char* errMsg)
{
    fprintf(stderr, "Error occurred when %s: %s\n", errMsg, strerror(errno));
    exit(1);
}

void shutdown()
{
    l_time = localtime( &m_time.tv_sec );
    
    printf( "%02d:%02d:%02d SHUTDOWN\n", l_time->tm_hour, l_time->tm_min, l_time->tm_sec );
    
    if(log_command == 1)
    {
        fprintf( logFiledes, "%02d:%02d:%02d SHUTDOWN\n", l_time->tm_hour, l_time->tm_min, l_time->tm_sec );
    }
    
    mraa_aio_close(tempSensor);
    mraa_gpio_close(button);
    
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
                fprintf(stdout, "Invalid Command\n");
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
        fprintf(stdout, "%s\n", temp);
        if (log_command == 1)
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
        {"period",     required_argument, NULL, 'p'},
        {"scale",    required_argument, NULL, 's'},
        {"log",     required_argument, NULL, 'l'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "p:s:l:", long_options, &optIndex)) != -1)
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
                    fprintf(stderr, "Usage: lab4b [--period=interval] [--scale={CF}] [--log=fileName] \n");
                    exit(1);
                }
            case 'l':
                log_command = 1;
                logFiledes = fopen(optarg, "w+");
                if (logFiledes == NULL)
                    Error("openning the log file");
                break;
            default:
                fprintf(stderr, "Usage: lab4b [--period=interval] [--scale={CF}] [--log=fileName] \n");
                exit(1);
        }
    }
    
    tempSensor = mraa_aio_init(1);
    if (tempSensor == NULL)
    {
        fprintf(stderr, "Failed to initialize tempSensor\n");
        mraa_deinit();
        exit(1);
    }
    button = mraa_gpio_init(60);
    if (button == NULL)
    {
        fprintf(stderr, "Failed to initialize button\n");
        mraa_deinit();
        exit(1);
    }
    
    mraa_gpio_dir(button, MRAA_GPIO_IN);
    
    int timeout = 0;
    
    struct pollfd fildes;
    fildes.fd = 0;
    fildes.events = POLLIN | POLLHUP | POLLERR;
    
    while (1)
    {
        stat = poll(&fildes, 1, timeout);
        if (stat < 0)
            Error("using poll system");
        else
        {
            stat = mraa_gpio_read(button);
            if (stat < 0)
            {
                fprintf(stderr, "Error occurred when reading button status\n");
                mraa_aio_close(tempSensor);
                mraa_gpio_close(button);
                exit(1);
            }
            else if (stat == 1)
            {
                shutdown();
            }
            
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
                        mraa_gpio_close(button);
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
                    
                    // if (report == 1)
                    // {
                    if (log_command == 1)
                    {
                        fprintf(stdout, "%02d:%02d:%02d %0.1f\n", l_time->tm_hour, l_time->tm_min, l_time->tm_sec, temp);
                    }
                    fprintf(logFiledes, "%02d:%02d:%02d %0.1f\n", l_time->tm_hour, l_time->tm_min, l_time->tm_sec, temp);
                    // }
                    n_time = m_time.tv_sec + interval;
                }
                
                
            }
            
            if( fildes.revents & POLLIN )
            {
                char chars[BUFFER_SIZE];
                int size = BUFFER_SIZE * sizeof(char);
                memset(chars, 0, size);
                stat = read(0, chars, size);
                if (stat < 0)
                    Error("reading from stdin");
                else
                {
                    int i;
                    char temp[100]="";
                    
                    for (i = 0; i < stat; i++)
                    {
                        if (chars[i] != '\n')
                        {
                            char c = chars[i];
                            strcat(temp, &c);
                        }
                        else
                        {
                            if (strcmp(temp, "SCALE=F") == 0 || strcmp(temp, "SCALE=C") == 0 )
                            {
                                opt_scale = temp[6];
                                fprintf(stdout, "%s\n", temp);
                                if (log_command == 1 && report == 1)
                                    fprintf(logFiledes, "%s\n", temp);
                            }
                            
                            else if (strcmp(temp, "START") == 0)
                            {
                                fprintf(stdout, "%s\n", temp);
                                if (log_command == 1)
                                    fprintf(logFiledes, "%s\n", temp);
                            }
                            else if (strcmp(temp, "STOP") == 0)
                            {
                                report = 0;
                                fprintf(stdout, "%s\n", temp);
                                if (log_command == 1)
                                    fprintf(logFiledes, "%s\n", temp);
                            }
                            else if (strcmp(temp, "OFF") == 0)
                            {
                                fprintf(stdout, "%s\n", temp);
                                if (log_command == 1)
                                    fprintf(logFiledes, "%s\n", temp);
                                
                                shutdown();
                            }
                            else
                            {
                                period_and_log(temp);
                            }
                            
                            strcpy(temp, "");
                        }
                        
                    }
                    
                }
            }
            
        }
    }
    
    mraa_aio_close(tempSensor);
    mraa_gpio_close(button);
    
    return 0;
}