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
#include <sys/types.h>
#include <sys/wait.h>

struct termios terminal_mode_ori;
int fromTerminal[2];
int toTerminal[2];
int BUFFER_SIZE = 256;
int shell = 0;
int stat;
int child_pid;

void Error(char* errMsg)
{
    fprintf(stderr, "Error occurred when %s: %s\n", errMsg, strerror(errno));
    exit(1);
}

void signal_handler(int n)
{
    if (shell && n == SIGINT)
        kill(child_pid, SIGINT);
    if (n == SIGPIPE)
        exit(0);
}

void restore_setting()
{
    stat = tcsetattr(0, TCSANOW, &terminal_mode_ori);
    if (stat < 0)
        Error("restore setting");
}

void echoInput(char* chars, int n, int outputFildes, int shell, int forward_shell)
{
    char temp[2] = {'\r','\n'};
    int temp_size = sizeof(char)*2;
    
    char temp1[1]={'\n'};
    
    int i;
    for (i = 0; i < n; i++)
    {
        //EOF: 0x04
        if (chars[i] == 0x04)
        {
            if (shell == 1)
                close(fromTerminal[1]);
            else
                exit(0);
        }

        //EOT: 0x03
        else if (chars[i] == 0x03)
        {
            if (shell == 1)
                kill(child_pid, SIGINT);
        }

        //CR: 0x0D, LF: 0x0A
        else if (chars[i] == 0x0D || chars[i] == 0x0A)
        {
            if (forward_shell == 1)
                write(outputFildes, &temp1, sizeof(char));
            else {
                write(outputFildes, &temp, temp_size);
            }
        }

        else
            write(outputFildes, chars+i, sizeof(char));
    }
}

void copy_keyboard()
{
    char chars[BUFFER_SIZE];

    //constantly taking keyboard inputs
    while (1)
    {
        stat = read(0, &chars, BUFFER_SIZE);
        if (stat < 0)
        {
            Error("reading from keyboard input");
        }

        echoInput(chars, stat, 1, 0, 0);
    }
}

void copy_shell()
{
    int timeout = 0;

    // Open STREAMS device.
    struct pollfd fildes[2];
    fildes[0].fd = 0;
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
            // Data may be read from terminal
            if (fildes[0].revents & POLLIN)
            {
                char chars_terminal[BUFFER_SIZE];
                int read_stat = read(0, chars_terminal, BUFFER_SIZE);
                if (read_stat < 0)
                    Error("reading from terminal");
                
                //echo input from terminal keyboard to standard output
                echoInput(chars_terminal, read_stat, 1, 1, 0);
                
                //forward input from terminal keyboard to shell
                echoInput(chars_terminal, read_stat, fromTerminal[1], 1, 1);
            }

            // Data may be read from shell
            if (fildes[1].revents & POLLIN)
            {
                char chars_shell[BUFFER_SIZE];
                int read_stat = read(fildes[1].fd, chars_shell, BUFFER_SIZE);
                if (read_stat < 0)
                    Error("reading from shell");
                echoInput(chars_shell, read_stat, 1, 1, 0);
            }

            //hangup or error
            if (fildes[1].revents & (POLLHUP | POLLERR))
            {
                kill(child_pid, SIGINT);
                close(fromTerminal[1]);
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
    // int debug = 0;

    static struct option long_options[] =
    {
        {"shell", no_argument, 0, 's'},
        {"debug", no_argument, 0, 'd'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "sd", long_options, &optIndex)) != -1)
      {
          switch (opt)
          {
              case 's':
                  shell = 1;
                  break;
              // case 'd':
              //     debug = 1;
              //     break;
              default:
                  fprintf(stderr, "Usage: lab1a [--shell] [--debug]\n");
                  exit(1);
          }
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

    if (shell)
    {
        signal(SIGPIPE, signal_handler);
        //build two pipes for both directions
        int stat1 = pipe(fromTerminal);
        int stat2 = pipe(toTerminal);
        if (stat1 == -1 || stat2 == -1)
            Error("building the pipe");

        child_pid = fork();
        if (child_pid == -1)
            Error("creating new process");
        else if (child_pid == 0)
            child_process();
        else
            parent_process();
    }
    else
        copy_keyboard();

    if (shell)
    {
        int status;
        stat = waitpid(child_pid, &status, 0);
        if (stat == -1)
            Error("executing waitpid");
        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
    }

    return 0;
}
