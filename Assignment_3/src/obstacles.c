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
#include <poll.h>
#include <errno.h>
#include <time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bits/sigaction.h>
#include "include/log.h"
#include "include/parameter.h"


// Declare global variables
volatile sig_atomic_t flag = 0; // Flag variable to inform process to clean memory, close links and quit 
int watchdog_pid; // Variable to store the pid of the watchdog
FILE *logfd; // File pointer that contains the file descriptor for the log file
char *logpath = "src/include/log/obstacles.log"; // Path for the log file
char address[250];
int port, seconds;
int obstacles2watchfd, watch2obstaclesfd; /// Fds associated with the created pipes
int obstacles_XPos[20], obstacles_YPos[20]; // Variables to hold obstacles positions
int dronePosition[2] = {15, 15}; // Variable to hold drone position
int n;
float window_size_y, window_size_x;
int max_y, max_x;
char *message;
int cmaxfd;


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
    A signal handler that sets the flag variable to 1 when receiving a terminate signal.
    */

    block_signal(SIGTERM);
    logmessage(logpath ,"Received SIGTERM\n");
    flag = 1;
    unblock_signal(SIGTERM);

}


void watchdog_signal_handler(int signum)
{
    /*
    A signal handler that sends a response back to the watchdog after 
    receiving a SIGUSR1 signal from it.
    */

    block_signal(SIGUSR1);
    kill(watchdog_pid, SIGUSR2);
    unblock_signal(SIGUSR1);

}


int main(int argc, char *argv[])
{
    // Parse the needed file descriptors from the arguments list
    sscanf(argv[1], "%d,%d,%s ,%d,%d", &obstacles2watchfd, &watch2obstaclesfd, address, &port, &seconds);


    // Create a log file and open it
    logopen(logpath);
    logint(logpath, "seconds", seconds);


    // Declare a signal handler to handle an INTERRUPT signal
    struct sigaction act;
    bzero(&act, sizeof(act));
    act.sa_handler = &terminate_signal_handler;
    sigaction(SIGTERM, &act, NULL); 


    // Declare a signal handle to handle the signals received from the watchdog
    struct sigaction act2;
    bzero(&act2, sizeof(act2));
    act2.sa_handler = &watchdog_signal_handler;
    sigaction(SIGUSR1, &act2, NULL);


    // Send the pid to the watchdog
    int pid = getpid();
    int pid_buf[1] = {pid};
    write(obstacles2watchfd, pid_buf, sizeof(pid_buf));
    logint(logpath, "Obstacles PID", pid);
    logmessage(logpath, "sent pid to watchdog");


    // Receive the watchdog PID
    int watchdog_list[1] = {0};
    read(watch2obstaclesfd, watchdog_list, sizeof(watchdog_list));
    watchdog_pid = watchdog_list[0];
    logmessage(logpath, "received watchdog pid");
    logint(logpath, "watchdog_pid", watchdog_pid);


    // Create a socket
    int network_socket;
    block_signal(SIGUSR1);
    network_socket = socket(AF_INET, SOCK_STREAM, 0);
    unblock_signal(SIGUSR1);
    if (network_socket < 0)
    {
        logerror(logpath, "Error: Opening socket", errno);
    }
    else
    {
        logmessage(logpath, "Opened socket");
    }


    // Specify an address for the socket
    struct sockaddr_in server_address;
    struct hostent *server;
    server_address.sin_family = AF_INET;
    server_address.sin_port =  htons(port); 
    server_address.sin_addr.s_addr = inet_addr(address);


    // Set the socket read to have a timeout
    // struct timeval timeout;
    // timeout.tv_sec = 1;
    // timeout.tv_usec = 0;
    // if (setsockopt(network_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    // {
    //     logerror(logpath, "Error: Setsockopt failed", errno);
    // }


    // Connect to the server
    block_signal(SIGUSR1);
    int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));
    unblock_signal(SIGUSR1);
    if (connection_status == -1)
    {
        logerror(logpath, "Error: Connection", errno);
    }
    else
    {
        logmessage(logpath, "Connected to server");
    }


    // Declare the buffers to be used to transmit messages
    char socket_write_buf[MAX_MSG_LEN];
    char socket_read_buf[MAX_MSG_LEN];


    // Send a message notifying the server this is the obstacle
    message = "OI";
    sprintf(socket_write_buf, "%s", message);
    logstring(logpath, "ID message", socket_write_buf);
    block_signal(SIGUSR1);
    n = write(network_socket, socket_write_buf, sizeof(socket_write_buf));
    unblock_signal(SIGUSR1);
    if (n < 0)
    {
        logerror(logpath, "Error: Socket write", errno);
    }
    else
    {
        logmessage(logpath, "Sent ID message");
         // Wait for an echo from the server
        block_signal(SIGUSR1);
        n = read(network_socket, socket_read_buf, sizeof(socket_read_buf));
        unblock_signal(SIGUSR1);
        if (n < 0)
        {
            logerror(logpath, "Error: Target ID echo", errno);
        }
        else
        {
            if (strcmp(socket_read_buf, socket_write_buf) == 0)
            {
                logmessage(logpath, "Read ID echo");
                logstring(logpath, "ID echo", socket_read_buf);
            }
            else
            {
                logerror(logpath, "Error: Wrong echo", errno);
                logstring(logpath, "Wrong ID echo", socket_read_buf);
            }
        }
    }


    // Get the window size from the server
    bzero(socket_write_buf, sizeof(socket_write_buf));
    bzero(socket_read_buf, sizeof(socket_read_buf));
    block_signal(SIGUSR1);
    n = read(network_socket, socket_read_buf, sizeof(socket_read_buf));
    unblock_signal(SIGUSR1);
    if (n < 0)
    {
        logerror(logpath, "Error: Window size read", errno);
    }
    else
    {
        logmessage(logpath, "Read window size");
        logstring(logpath, "Window size", socket_read_buf);
        block_signal(SIGUSR1);
        n = write(network_socket, socket_read_buf, sizeof(socket_read_buf));
        unblock_signal(SIGUSR1);
        if (n < 0)
        {
            logerror(logpath, "Error: Window size echo", errno);
        }
        else
        {
            logmessage(logpath, "Sent window size echo");
        }
    }


    // Parse the window size from the buffer
    sscanf(socket_read_buf, "%f,%f", &window_size_x, &window_size_y);
    logdouble(logpath, "window_size_x", window_size_x);
    logdouble(logpath, "window_size_y", window_size_y);
    max_x = (int)window_size_x;
    max_y = (int)window_size_y;
    logint(logpath, "max_x", max_x);
    logint(logpath, "max_y", max_y);


    // Declare a variable to count the number of cycles before the obstacles are refreshed
    int refresh_count = 100;
    int obstacle_count = 0;


    while(1)
    {
        // Log the completion of a cycle
        logmessage(logpath, "Completed 1 cycle");


        // Create a fd set for the socket
        fd_set cfds;
        struct timeval tval;
        FD_ZERO(&cfds);
        FD_SET(network_socket, &cfds);
        tval.tv_sec = 0;
        tval.tv_usec = 10000;
        cmaxfd = network_socket;


        // Reset the socket buffers
        bzero(socket_write_buf, sizeof(socket_write_buf));
        bzero(socket_read_buf, sizeof(socket_read_buf));

        logmessage(logpath, "1");

        // Check for new data on the socket
        block_signal(SIGUSR1);
        int value = select((cmaxfd + 1), &cfds, NULL, NULL, &tval);
        unblock_signal(SIGUSR1);
        if (value == -1)
        {
            logerror(logpath, "Error: select", errno);
        }

        logmessage(logpath, "2");


        // If new data is available, read and perform the necessary actions
        if (FD_ISSET(network_socket, &cfds))
        {
            // Read the message from the server
            block_signal(SIGUSR1);
            n = read(network_socket, socket_read_buf, sizeof(socket_read_buf));
            unblock_signal(SIGUSR1);
            if (n < 0)
            {
                logerror(logpath, "Error: Socket read", errno);
            }
            else
            {
                logmessage(logpath, "Read request");
                logstring(logpath, "Request", socket_read_buf);
                block_signal(SIGUSR1);
                n = write(network_socket, socket_read_buf, sizeof(socket_read_buf));
                unblock_signal(SIGUSR1);
                if (n < 0)
                {
                    logerror(logpath, "Error: Socket write", errno);
                }
                else
                {
                    logmessage(logpath, "Sent request echo");
                }
            }
        }


        // If STOP send a message to the watchdog to end all the processes
        if ((strcmp(socket_read_buf, "STOP"))==0)
        {
            // Send a SIGUSR2 to the watchdog to kill all the processes
            logmessage(logpath, "Received STOP message");
            block_signal(SIGUSR1);
            kill(watchdog_pid, SIGUSR1);
            unblock_signal(SIGUSR1);
        }


        // Generate a random number to represent the number of obstacles
        srandom(time(NULL));
        while ((obstacle_count < 5) || (obstacle_count > 20))
        {
            obstacle_count = random() % MAX_COUNT;
        }
        logint(logpath, "obstacle_count", obstacle_count);


        // Generate random x and y positions for each target in a target array
        for (int i = 0; i < obstacle_count; i++)
        {
            int y = random() % max_y;
            int x = random() % max_x;
            obstacles_YPos[i] = y;
            obstacles_XPos[i] = x;
        }


        // Encode the obstacle positions into a message string
        bzero(socket_write_buf, sizeof(socket_write_buf));
        bzero(socket_read_buf, sizeof(socket_read_buf));
        char count[250];
        char *seperator1 = ",";
        char *seperator2 = "|";
        sprintf(count, "T[%d]", obstacle_count);
        strcat(socket_write_buf, count);
        for (int i = 0; i < obstacle_count; i++)
        {
            char pos[400];
            sprintf(pos, "%.3f,%.3f", (float)obstacles_XPos[i], (float)obstacles_YPos[i]);
            strcat(socket_write_buf, pos);
            if (i != (obstacle_count - 1))
            {
                strcat(socket_write_buf, seperator2);
            }
        }


        // Send the obstacle positions to the server
        block_signal(SIGUSR1);
        n = write(network_socket, socket_write_buf, sizeof(socket_write_buf));
        unblock_signal(SIGUSR1);
        if (n < 0)
        {
            logerror(logpath, "Error: Obstacle_pos write", errno);
        }
        else
        {
            logmessage(logpath, "Sent obstacle_pos");
            // Wait for an echo from the server
            block_signal(SIGUSR1);
            n = read(network_socket, socket_read_buf, sizeof(socket_read_buf));
            unblock_signal(SIGUSR1);
            if (n < 0)
            {
                logerror(logpath, "Error: Obstacle_pos echo", errno);
            }
            else
            {
                if (strcmp(socket_read_buf, socket_write_buf) == 0)
                {
                    logmessage(logpath, "Read obstacle_pos echo");
                }
                else
                {
                    logerror(logpath, "Error: Wrong obstacle_pos echo", errno);
                    exit(0);
                }
            }
        }


        block_signal(SIGTERM);
        if (flag == 1)
        {
            logmessage(logpath, "closing links and pipes");


            // Close the network_socket socket
            if((close(network_socket)) == -1)
            {
                logerror(logpath, "error: network_socket close", errno);
            }
            else
            {
                logmessage(logpath, "Closed network_socket");
            }


            // Close the fd that sends obstacle PID to watchdog
            if((close(obstacles2watchfd)) == -1)
            {
                logerror(logpath, "error: obstacles2watchfd close", errno);
            }
            else
            {
                logmessage(logpath, "Closed obstacles2watchfd");
            }


            // Close the fd that receives watchdog PID 
            if((close(watch2obstaclesfd)) == -1)
            {
                logerror(logpath, "error: watch2obstaclesfd close", errno);
            }
            else
            {
                logmessage(logpath, "Closed watch2obstaclesfd");
            }


            // Close the network_socket socket
            if((close(network_socket)) == -1)
            {
                logerror(logpath, "error: network_socket close", errno);
            }
            else
            {
                logmessage(logpath, "Closed network_socket");
            }
            

            // Kill the konsole and then self
            logmessage(logpath, "Killing process");
            kill(getppid(), SIGINT);
            struct sigaction act;
            bzero(&act, sizeof(act));
            act.sa_handler = SIG_DFL;
            sigaction(SIGTERM, &act, NULL);
            raise(SIGTERM);

            return -1;
        }
        unblock_signal(SIGTERM);

        // Time interval one cycle is to be run at
        struct timespec req;
        req.tv_nsec = 0;
        req.tv_sec = seconds;
        struct timespec rem;
        while(nanosleep(&req, &rem) != 0)
        {
            req.tv_nsec = rem.tv_nsec;
            req.tv_sec = rem.tv_sec;
            rem.tv_nsec = 0;
            rem.tv_sec = 0;
        }
    }
}