// Name: Jingjing 
// Email: 
// ID:

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include "SortedList.h"

long num_thread = 1;
long num_iteration = 1;
long num_element;
long long counter;
int stat;
int opt_yield = 0;
int opt_yield;
char* opt_sync = NULL;
char* yieldArg = NULL;
SortedList_t *header;
SortedListElement_t **element;
pthread_mutex_t mutexThread = PTHREAD_MUTEX_INITIALIZER;
int spin = 0;
#define BILLION 1000000000L

void Error(char* errMsg)
{
    fprintf(stderr, "Error occurred when %s: %s\n", errMsg, strerror(errno));
    exit(1);
}

void signal_handler(int n)
{
    if (n == SIGSEGV)
        Error("handling the signal");
}

void element_initialize (SortedListElement_t **element, int size)
{
    int key_len = 10;

    srand((unsigned int) time(NULL));
    int i;
    for (i = 0; i < size; i++)
    {
        element[i] = malloc(sizeof(SortedListElement_t));
        element[i]->prev = NULL;
        element[i]->next = NULL;

        char *all_chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        char *rand_key = malloc(sizeof(char) * (key_len + 1));
        int j;
        for (j = 0; j < key_len; j++)
        {
            rand_key[j] = all_chars[rand() % (sizeof(all_chars)-1)];
        }
        rand_key[key_len] = '\0';
        element[i]->key = rand_key;
    }
}

void* thread_operations(void * arg)
{
    int thread_id = *(int *)arg;

    int i;
    for (i = thread_id; i < num_element; i+= num_thread)
    {
        if (opt_sync != NULL && *opt_sync == 'm')
            pthread_mutex_lock(&mutexThread);
        
        else if (opt_sync != NULL && *opt_sync =='s')
        {
            while (__sync_lock_test_and_set(&spin, 1));
        }
        
        SortedList_insert(header, (SortedListElement_t *) (element[i]));
        
        if (opt_sync != NULL && *opt_sync == 'm')
            pthread_mutex_unlock(&mutexThread);
        else if (opt_sync != NULL && *opt_sync == 's')
            __sync_lock_release(&spin);
    }

    if (opt_sync != NULL && *opt_sync == 'm')
        pthread_mutex_lock(&mutexThread);
    
    else if (opt_sync != NULL && *opt_sync =='s')
    {
        while (__sync_lock_test_and_set(&spin, 1));
    }
    
    long length = SortedList_length(header);
    if (length == -1)
        Error("inserting elements to the list");
    
    if (opt_sync != NULL && *opt_sync == 'm')
        pthread_mutex_unlock(&mutexThread);
    else if (opt_sync != NULL && *opt_sync == 's')
        __sync_lock_release(&spin);

    for (i = thread_id; i < num_element; i+= num_thread)
    {
        if (opt_sync != NULL && *opt_sync == 'm')
            pthread_mutex_lock(&mutexThread);
        
        else if (opt_sync != NULL && *opt_sync =='s')
        {
            while (__sync_lock_test_and_set(&spin, 1));
        }
        
        SortedListElement_t * element_found = SortedList_lookup(header, element[i]->key);
        if (element_found == NULL)
            Error("looking up the element in the list");

        stat = SortedList_delete(element_found);
        if (stat != 0)
            Error("deleting the element from the list");
        
        if (opt_sync != NULL && *opt_sync == 'm')
            pthread_mutex_unlock(&mutexThread);
        else if (opt_sync != NULL && *opt_sync == 's')
            __sync_lock_release(&spin);
    }

    return NULL;
}

void print_results(char *testName, long num_thread, long num_iteration, long runTime)
{
    long operNum = 3*num_thread*num_iteration;
    long runTime_avg = runTime/operNum;
    int num_list = 1;

    printf("%s,%ld,%ld,%d,%ld,%ld,%ld\n", testName, num_thread, num_iteration, num_list, operNum, runTime, runTime_avg);
}

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

char* get_test_name()
{
    char* testName;

    if (yieldArg != NULL)
        testName = concat("list-", yieldArg);
    else
        testName = "list-none";

    if (opt_sync != NULL)
    {
        testName = concat(testName, "-");
        testName = concat(testName, opt_sync);
    }
    else
    {
        testName = concat(testName, "-none");
    }

    return testName;
}

int main(int argc, char *argv[])
{
    int opt, optIndex, i;
    counter = 0;

    static struct option long_options[] =
    {
        {"threads",     required_argument, 0, 't'},
        {"iterations",  required_argument, 0, 'i'},
        {"yield",       required_argument, 0, 'y'},
        {"sync",        required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "t:i:y:s:", long_options, &optIndex)) != -1)
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
                for(i = 0; i < (int)strlen(optarg); i++)
                {
                    if(optarg[i] == 'i')
                        opt_yield |= INSERT_YIELD;
                    else if(optarg[i] == 'd')
                        opt_yield |= DELETE_YIELD;
                    else if(optarg[i] == 'l')
                        opt_yield |= LOOKUP_YIELD;
                    else
                    {
                        fprintf(stderr, "Usage: lab2_add [--threads=num_thread] [--iterations=num_iteration] [--yield={idl}] [--sync={ms}]\n");
                        exit(1);
                    }
                }
                yieldArg = optarg;
                break;
            case 's':
                opt_sync = optarg;
                break;
            default:
                fprintf(stderr, "Usage: lab2_add [--threads=num_thread] [--iterations=num_iteration] [--yield={idl}] [--sync={ms}]\n");
                exit(1);
        }
    }

    signal(SIGSEGV, signal_handler);

    header = malloc(sizeof(SortedList_t));
    header->next = header;
    header->prev = header;
    header->key = NULL;

    num_element = num_iteration * num_thread;
    element = malloc(num_element * sizeof(SortedListElement_t));
    element_initialize(element, num_element);

    pthread_t *thread;
    thread = malloc(num_thread * sizeof(pthread_t));
    if (thread == NULL)
        Error("allocating memory for threads");

    struct timespec start, stop;

    stat = clock_gettime(CLOCK_MONOTONIC, &start);
    if( stat == -1 )
        Error("getting the time");

    int thread_id[num_thread];
    for (i = 0; i < num_thread; i++)
    {
        thread_id[i] = i;
        stat = pthread_create(&thread[i], NULL, thread_operations, &thread_id[i]);

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

    if (SortedList_length(header) != 0)
        Error("calculating the list length at the end");

    long time_diff = BILLION * (stop.tv_sec - start.tv_sec) + stop.tv_nsec - start.tv_nsec;

    char* testName = get_test_name();
    print_results(testName,num_thread,num_iteration,time_diff);

    free(thread);
    free(element);
    free(header);

    return 0;
}

