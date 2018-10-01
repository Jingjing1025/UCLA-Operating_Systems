// Name: Jingjing 
// Email: 
// ID:

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>

long num_thread = 1;
long num_iteration = 1;
long long counter;
int stat;
int opt_yield = 0;
char* opt_sync = NULL;
#define BILLION 1000000000L

void Error(char* errMsg)
{
    fprintf(stderr, "Error occurred when %s: %s\n", errMsg, strerror(errno));
    exit(1);
}

void add(long long *pointer, long long value)
{
    long long sum = *pointer + value;
    if (opt_yield)
        sched_yield();
    *pointer = sum;
}

pthread_mutex_t mutexThread;
void add_m(long long *pointer, long long value)
{
    pthread_mutex_lock(&mutexThread);
    add(pointer, value);
    pthread_mutex_unlock(&mutexThread);
}

int spin = 0;
void add_s(long long *pointer, long long value)
{
    while (__sync_lock_test_and_set(&spin, 1));
    add(pointer, value);
    __sync_lock_release(&spin);
}

void add_c(long long *pointer, long long value)
{
    long long old, new_sum;
    do {
        old = *pointer;
        new_sum = old + value;
        if (opt_yield)
            sched_yield();
    } while (__sync_val_compare_and_swap(pointer, old, new_sum) != old);
}


void* thread_operations(void * arg)
{
    int i;
    for (i = 0; i < num_iteration; i++)
    {
        if (opt_sync != NULL && *opt_sync == 'm')
            add_m(arg, 1);
        else if (opt_sync != NULL && *opt_sync == 's')
            add_s(arg, 1);
        else if (opt_sync != NULL && *opt_sync == 'c')
            add_c(arg, 1);
        else
            add(arg, 1);
    }
    
    for (i = 0; i < num_iteration; i++)
    {
        if (opt_sync != NULL && *opt_sync == 'm')
            add_m(arg, -1);
        else if (opt_sync != NULL && *opt_sync == 's')
            add_s(arg, -1);
        else if (opt_sync != NULL && *opt_sync == 'c')
            add_c(arg, -1);
        else
            add(arg, -1);
    }
    return NULL;
}

void print_results(char *testName, long num_thread, long num_iteration, long runTime, long long counter)
{
    long operNum = 2*num_thread*num_iteration;
    long runTime_avg = runTime/operNum;
    
    printf("%s,%ld,%ld,%ld,%ld,%ld,%lld\n", testName, num_thread, num_iteration, operNum, runTime, runTime_avg, counter);
}

char* get_test_name()
{
    char* testName;
    if (opt_yield)
    {
        if (opt_sync != NULL && opt_sync[0]=='m')
            testName = "add-yield-m";
        else if (opt_sync != NULL && opt_sync[0]=='s')
            testName = "add-yield-s";
        else if (opt_sync != NULL && opt_sync[0]=='c')
            testName = "add-yield-c";
        else
            testName = "add-yield-none";
    }
    else
    {
        if (opt_sync != NULL && opt_sync[0]=='m')
            testName = "add-m";
        else if (opt_sync != NULL && opt_sync[0]=='s')
            testName = "add-s";
        else if (opt_sync != NULL && opt_sync[0]=='c')
            testName = "add-c";
        else
            testName = "add-none";
    }
    
    return testName;
}

int main(int argc, char *argv[])
{
    int opt, optIndex;
    counter = 0;
    
    static struct option long_options[] =
    {
        {"threads",     required_argument, 0, 't'},
        {"iterations",  required_argument, 0, 'i'},
        {"yield",       no_argument,       0, 'y'},
        {"sync",        required_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "t:i:ys:", long_options, &optIndex)) != -1)
    {
        switch (opt)
        {
            case 't':
                num_thread = atoi(optarg);
                break;
            case 'i':
                num_iteration = atoi(optarg);
                break;
            case 'y':
                opt_yield = 1;
                break;
            case 's':
                opt_sync = optarg;
                break;
            default:
                fprintf(stderr, "Usage: lab2_add [--threads=num_thread] [--iterations=num_iteration]\n");
                exit(1);
        }
    }
    
    pthread_t *thread;
    thread = malloc(num_thread * sizeof(pthread_t));
    if (thread == NULL)
        Error("allocating memory for threads");
    
    struct timespec start, stop;
    
    stat = clock_gettime(CLOCK_MONOTONIC, &start);
    if( stat == -1 )
        Error("getting the time");
    
    int i;
    for (i = 0; i < num_thread; i++)
    {
        stat = pthread_create(&thread[i], NULL, thread_operations, &counter);
        if (stat != 0)
            Error("creating the threads");
    }
    
    for (i = 0; i < num_thread; i++)
    {
        pthread_join(thread[i], NULL);
    }
    
    stat = clock_gettime(CLOCK_MONOTONIC, &stop);
    if( stat == -1 )
        Error("getting the time");
    
    long time_diff = BILLION * (stop.tv_sec - start.tv_sec) + stop.tv_nsec - start.tv_nsec;
    char* testName = get_test_name();
    
    print_results(testName,num_thread,num_iteration,time_diff,counter);
    
    free(thread);
    
    return 0;
}
