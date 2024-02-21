#include <stdio.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <signal.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <bits/sigaction.h>
#include "include/log.h"
#include "include/parameter.h"

// Define global variables
volatile sig_atomic_t flag = 0; // Flag variable to inform watchdog that user has closed the program and to terminate all processes 
volatile sig_atomic_t received_response = 0; // Flag that sets when a response is received
int server_buf[1] = {0}; // A buffer to hold the server pid
int ui_buf[1] = {0}; // A buffer to hold the UI pid
int drone_buf[1] = {0}; // A buffer to hold the drone pid
int targets_buf[1] = {0}; // A buffer to hold the targets pid
int obstacles_buf[1] = {0}; // A buffer to hold the obstacles pid
FILE *logfd; // File pointer that contains the file descriptor for the log file
char *logpath = "src/include/log/watchdog.log"; // Path for the log file
char type[2]; // Variable to hold the char distinguishing if the execution is a server or client

void block_signal(int signal)
{
    /*
    A function that blocks the signal specified in the function argument.
    */

    // Create a set of signals to block
    sigset_t sigset;

    // Initialize the set to 0
    sigemptyset(&sigset);

    // Add the signal to the set
    sigaddset(&sigset, signal);

    // Add the signals in the set to the process' blocked signals
    sigprocmask(SIG_BLOCK, &sigset, NULL);
}


void unblock_signal(int signal)
{
    /*
    A function that unblocks the signal specified in the function argument.
    */
   
    // Create a set of signals to unblock
    sigset_t sigset;

    // Initialize the set to 0
    sigemptyset(&sigset);

    // Add the signal to the set
    sigaddset(&sigset, signal);

    // Remove the signals in the set from the process' blocked signals
    sigprocmask(SIG_UNBLOCK, &sigset, NULL);
}


void terminate_signal_handler(int signum)
{
    /* 
    A signal handler that shuts down the system when receiving 
    SIGUSR1 from the drone or target.
    */

    block_signal(SIGUSR1);
    logmessage(logpath, "Received SIGUSR1");
    flag = 1;
    unblock_signal(SIGUSR1);

}


void watchdog_signal_handler(int signum)
{
    /*
    A signal handler that changes the received_response flag variable when
    receiving a response from a process.
    */

    block_signal(SIGUSR2);
    received_response = 1;
    unblock_signal(SIGUSR2);

}


int main(int argc, char *argv[])
{
    // Parse the needed file descriptors from the arguments list
    int server2watchfd, UI2watchfd, drone2watchfd, watch2serverfd, watch2UIfd, watch2dronefd, targets2watchfd, watch2targetsfd, obstacles2watchfd, watch2obstaclesfd;
    sscanf(argv[1], "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s ", &server2watchfd, &UI2watchfd, &drone2watchfd, &watch2serverfd, &watch2UIfd, &watch2dronefd, &targets2watchfd, &watch2targetsfd, &obstacles2watchfd, &watch2obstaclesfd, type);


    // Print the current working directory
    char cwd[100];
    getcwd(cwd, sizeof(cwd));
    printf("Current working directory: %s\n", cwd);

    
    // Open the log file
    logopen(logpath);
    

    // Declare the required variables
    int received_pids = 0;


    // Declare a signal handler to handle a SIGUSR2 signal sent from the processes
    struct sigaction act;
    bzero(&act, sizeof(act));
    act.sa_handler = &watchdog_signal_handler;
    sigaction(SIGUSR2, &act, NULL);


    // Declare a signal handler to handle a SIGUSR1 signal sent from the drone to shutdown the system
    struct sigaction act2;
    bzero(&act2, sizeof(act2));
    act2.sa_handler = &terminate_signal_handler;
    sigaction(SIGUSR1, &act2, NULL);


    // Find the maxfd from the current set of fds
    int maxfd = -1;
    int fd[5] = {server2watchfd, UI2watchfd, drone2watchfd, targets2watchfd, obstacles2watchfd};
    for (int i = 0; i < 5; i++)
    {
        if (fd[i] > maxfd)
        {
            maxfd = fd[i];
        }
    }


    // Introduce a small delay to allow the processes to send the PIDs
    sleep(6);


    // Receive the pids from every process
    fd_set rfds;
    struct timeval tv;
    FD_ZERO(&rfds);
    FD_SET(server2watchfd, &rfds);
    FD_SET(UI2watchfd, &rfds);
    FD_SET(drone2watchfd, &rfds);
    FD_SET(targets2watchfd, &rfds);
    FD_SET(obstacles2watchfd, &rfds);
    tv.tv_usec = 10000;
    tv.tv_sec = 0;
    int retval = select((maxfd+1), &rfds, NULL, NULL, &tv);
    if (retval == -1)
    {
        logerror(logpath, "error: select", errno);
    }


    // Retrieve the pids of the processes
    int pidlist[3];
    for (int i = 0; i < 5; i++)
    {
        if(FD_ISSET(fd[i], &rfds))
        {
            if (fd[i] == server2watchfd)
            {
                read(server2watchfd, server_buf, sizeof(server_buf));
                received_pids = received_pids + 1;
            }
            else if (fd[i] == UI2watchfd)
            {
                read(UI2watchfd, ui_buf, sizeof(ui_buf));
                received_pids = received_pids + 1;
            }
            else if (fd[i] == drone2watchfd)
            {
                read(drone2watchfd, drone_buf, sizeof(drone_buf));
                received_pids = received_pids + 1;
            }
            else if (fd[i] == targets2watchfd)
            {
                read(targets2watchfd, targets_buf, sizeof(targets_buf));
                received_pids = received_pids + 1;
            }
            else if (fd[i] == obstacles2watchfd)
            {
                read(obstacles2watchfd, obstacles_buf, sizeof(obstacles_buf));
                received_pids = received_pids + 1;
            }
        }
    }
    if ((strcmp(type, "S"))==0)
    {
        pidlist[0] = server_buf[0];
        pidlist[1] = ui_buf[0];
        pidlist[2] = drone_buf[0];
        logint(logpath, "server pid", pidlist[0]);
        logint(logpath, "UI pid", pidlist[1]);
        logint(logpath, "drone pid", pidlist[2]);
    }
    else if ((strcmp(type, "C"))==0)
    {
        pidlist[0] = targets_buf[0];
        pidlist[1] = obstacles_buf[0];
        logint(logpath, "targets pid", pidlist[0]);
        logint(logpath, "obstacles pid", pidlist[1]);
    }


    // Send the watchdog PID to the processes
    int pid = getpid();
    int pid_buf[1] = {pid};
    if ((strcmp(type, "S"))==0)
    {
        write(watch2serverfd, pid_buf, sizeof(pid_buf));
        write(watch2UIfd, pid_buf, sizeof(pid_buf));
        write(watch2dronefd, pid_buf, sizeof(pid_buf));
    }
    else if ((strcmp(type, "C"))==0)
    {
        write(watch2targetsfd, pid_buf, sizeof(pid_buf));
        write(watch2obstaclesfd, pid_buf, sizeof(pid_buf));
    }
    logint(logpath, "watchdog pid", pid);
    logmessage(logpath, "sent watchdog pid to processes");


    // Introduce a small delay to allow the processes to read the pipes
    sleep(1);


    // Declare the variables to be used in the watchdog
    int cycles = 0;
    int current_pid;


    while(1)
    {
        // If the flag is set, close all links and pipes and terminate the process 
        block_signal(SIGUSR1);
        if (flag == 1)
        {
            if ((strcmp(type, "S"))==0)
            {
                kill(ui_buf[0], SIGTERM);
                kill(server_buf[0], SIGTERM);
                kill(drone_buf[0], SIGTERM);
            }
            else if ((strcmp(type, "C"))==0)
            {
                kill(targets_buf[0], SIGTERM);
                kill(obstacles_buf[0], SIGTERM);
            }
            kill(getppid(), SIGINT);
        }
        unblock_signal(SIGUSR1);


        // Iterate through the pid list and check processes health
        for (int i = 0; i < 3; i++)
        {
            // Check if the maximum cycles has been reached
            if (cycles == 15)
            {
                 // Send a kill signal to every pid in pidlist 
                    char str[100];
                    sprintf(str, "No response from %d: KILLING ALL PROCESSES", current_pid);
                    logmessage(logpath, str);
                    if ((strcmp(type, "S"))==0)
                    {
                        logmessage(logpath, "Killed socket server processes");
                        kill(ui_buf[0], SIGTERM);
                        kill(server_buf[0], SIGTERM);
                        kill(drone_buf[0], SIGTERM);
                    }
                    else if ((strcmp(type, "C"))==0)
                    {
                        logmessage(logpath, "Killed socket client processes");
                        kill(targets_buf[0], SIGTERM);
                        kill(obstacles_buf[0], SIGTERM);
                    }


                    // Close the fd for the server pid
                    if((close(server2watchfd)) == -1)
                    {
                        logerror(logpath, "error: server2watchfd close", errno);
                    }
                    else
                    {
                        logmessage(logpath, "Closed server2watchfd");
                    }

                    // Close the fd for the UI pid
                    if((close(UI2watchfd)) == -1)
                    {
                        logerror(logpath, "error: UI2watchfd close", errno);
                    }
                    else
                    {
                        logmessage(logpath, "Closed UI2watchfd");
                    }

                    // Close the fd for the drone pid
                    if((close(drone2watchfd)) == -1)
                    {
                        logerror(logpath, "error: drone2watchfd close", errno);
                    }
                    else
                    {
                        logmessage(logpath, "Closed drone2watchfd");
                    }


                    // Close the fd that sends watchdog pid to the server
                    if((close(watch2serverfd)) == -1)
                    {
                        logerror(logpath, "error: watch2serverfd close", errno);
                    }
                    else
                    {
                        logmessage(logpath, "Closed watch2serverfd");
                    }


                    // Close the fd that sends watchdog pid to the UI
                    if((close(watch2UIfd)) == -1)
                    {
                        logerror(logpath, "error: watch2UIfd close", errno);
                    }
                    else
                    {
                        logmessage(logpath, "Closed watch2UIfd");
                    }


                    // Close the fd that sends watchdog pid to the drone
                    if((close(watch2dronefd)) == -1)
                    {
                        logerror(logpath, "error: watch2dronefd close", errno);
                    }
                    else
                    {
                        logmessage(logpath, "Closed watch2dronefd");
                    }


                    // Close the fd for the targets pid
                    if((close(targets2watchfd)) == -1)
                    {
                        logerror(logpath, "error: targets2watchfd close", errno);
                    }
                    else
                    {
                        logmessage(logpath, "Closed targets2watchfd");
                    }
                    

                    // Close the fd that sends watchdog pid to the drone
                    if((close(watch2targetsfd)) == -1)
                    {
                        logerror(logpath, "error: watch2targetsfd close", errno);
                    }
                    else
                    {
                        logmessage(logpath, "Closed watch2targetsfd");
                    }


                    // Close the fd for the targets pid
                    if((close(obstacles2watchfd)) == -1)
                    {
                        logerror(logpath, "error: obstacles2watchfd close", errno);
                    }
                    else
                    {
                        logmessage(logpath, "Closed obstacles2watchfd");
                    }
                    

                    // Close the fd that sends watchdog pid to the drone
                    if((close(watch2obstaclesfd)) == -1)
                    {
                        logerror(logpath, "error: watch2obstaclesfd close", errno);
                    }
                    else
                    {
                        logmessage(logpath, "Closed watch2obstaclesfd");
                    }


                    // Kill the konsole and self
                    kill(getppid(), SIGINT);
                    raise(SIGTERM);
            }


            // Iterate through every pid in pidlist
            if (((strcmp(type, "C"))==0) && (i == 2))
            {
                continue;
            }
            else
            {
                current_pid = pidlist[i];
            }
            logint(logpath, "current pid", current_pid);


            // Send a signal to the pid
            char str[100];
            kill(current_pid, SIGUSR1);
            sprintf(str, "sent SIGUSR1 to %d", current_pid);
            logmessage(logpath, str);
            usleep(50);


            // Wait for a certain number of cycles
            while (cycles < 15)
            {
                // Wait a certain duration between cycles
                usleep(DELTA_TIME_USEC);

                // Check if the flag variable has changed
                if (received_response == 1)
                {
                    // Reset all variables and move to next pid
                    sprintf(str, "received response from %d", current_pid);
                    logmessage(logpath, str);
                    cycles = 0;
                    received_response = 0;
                    break;
                }
                else
                {
                    // Increment cycle count
                    cycles++;
                }
                logint(logpath, "cycles", cycles);
            }
        }
    }

    return 0;
}