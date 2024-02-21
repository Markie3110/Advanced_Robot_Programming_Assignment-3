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
char *logpath = "src/include/log/targets.log"; // Path for the log file
int targets2watchfd, watch2targetsfd; // Fds associated with the created pipes
char address[250] = ""; // Variable to hold the server address
int port; // Variable to hold the port number
int targets_XPos[20], targets_YPos[20]; // Variables to hold targets positions
int dronePosition[2] = {15, 15}; // Variable to hold drone position
int n; // Variable to store the values returned by the function read_and_echo and write_and_echos
char *message; // Variable to hold TI and OI strings


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
    unblock_signal(SIGTERM);
    flag = 1;

}


void watchdog_signal_handler(int signum)
{
    /*
    A signal handler that sends a response back to the watchdog after 
    receiving a SIGUSR1 signal from it.
    */

    block_signal(SIGUSR1);
    kill(watchdog_pid, SIGUSR2);
    block_signal(SIGUSR1);

}


int read_and_echo(int fd, char *readbuf, int readbufsize, char *message1, char *message2, char *message3, char *message4)
{
    /*
    A function that reads from the socket and then echos back the message.
    */

    // Read from the socket
    bzero(readbuf, readbufsize);
    int n1, n2;
    block_signal(SIGUSR1);
    n1 = read(fd, readbuf, readbufsize);
    unblock_signal(SIGUSR1);
    if (n1 < 0)
    {
        logerror(logpath, message1, errno);
    }
    else
    {
        // Echo the message
        logmessage(logpath, message2);
        block_signal(SIGUSR1);
        n2 = write(fd, readbuf, readbufsize);
        unblock_signal(SIGUSR1);
        if (n2 < 0)
        {
            logerror(logpath, message3, errno);
        }
        else
        {
            logmessage(logpath, message4);
        }
    }

    return n1;
}


int write_and_echo(int fd, char *writebuf, char *readbuf, int writebufsize, int readbufsize, char *message1, char *message2, char *message3, char *message4, char *message5)
{
    /*
    A function that writes to a socket and then monitors the echo, logging if it is true to the original or not.
    */

    // Write to the socket
    block_signal(SIGUSR1);
    int n1, n2;
    n1 = write(fd, writebuf, writebufsize);
    unblock_signal(SIGUSR1);
    if (n1 < 0)
    {
        logerror(logpath, message1, errno);
    }
    else
    {
        // Wait for the echo from client
        logmessage(logpath, message2);
        bzero(readbuf, readbufsize);
        block_signal(SIGUSR1);
        n2 = read(fd, readbuf, readbufsize);
        unblock_signal(SIGUSR1);
        if (n2 < 0)
        {
            logerror(logpath, message3, errno);
        }
        else
        {
            if (strcmp(readbuf, writebuf) == 0)
            {
                logmessage(logpath, message4);
            }
            else
            {
                logerror(logpath, message5, errno);
            }
        }
    }

    return n2;
}


int main(int argc, char *argv[])
{
    // Parse the needed file descriptors from the arguments list
    sscanf(argv[1], "%d,%d,%s ,%d", &targets2watchfd, &watch2targetsfd, address, &port);


    // Create a log file and open it
    logopen(logpath);


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
    write(targets2watchfd, pid_buf, sizeof(pid_buf));
    logint(logpath, "Targets PID", pid);
    logmessage(logpath, "sent pid to watchdog");


    // Receive the watchdog PID
    int watchdog_list[1] = {0};
    read(watch2targetsfd, watchdog_list, sizeof(watchdog_list));
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


    // Send a message notifying the server this is the target
    message = "TI";
    sprintf(socket_write_buf, "%s", message);
    logstring(logpath, "ID message", socket_write_buf);
    n = write_and_echo(network_socket, socket_write_buf, socket_read_buf, sizeof(socket_write_buf), sizeof(socket_read_buf), "Error: Socket write", "Sent ID message", "Error: Socket read", "Read ID echo", "Error: Wrong echo");
    if (n > 0)
    {
        logstring(logpath, "ID echo", socket_read_buf);
    }
    else if (n < 0)
    {
        logstring(logpath, "Wrong ID echo", socket_read_buf);
    }


    // Get the window size from the server
    bzero(socket_write_buf, sizeof(socket_write_buf));
    bzero(socket_read_buf, sizeof(socket_read_buf));
    n = read_and_echo(network_socket, socket_read_buf, sizeof(socket_read_buf), "Error: Window size read", "Read window size", "Error: Window size echo", "Sent window size echo");
    if (n > 0)
    {
        logstring(logpath, "Window size", socket_read_buf);
    }


    // Parse the window size from the buffer
    float window_size_y, window_size_x;
    int max_y, max_x; 
    sscanf(socket_read_buf, "%f,%f", &window_size_x, &window_size_y);
    logdouble(logpath, "window_size_x", window_size_x);
    logdouble(logpath, "window_size_y", window_size_y);
    max_x = (int)window_size_x;
    max_y = (int)window_size_y;
    logint(logpath, "max_x", max_x);
    logint(logpath, "max_y", max_y);


    // Generate a random number to represent the number of targets
    int target_count = 0;
    srandom(time(NULL)*1000);
    while ((target_count < 5) || (target_count > 20))
    {
        target_count = random() % MAX_COUNT;
    }
    logint(logpath, "target_count", target_count);


    // Generate random x and y positions for each target in a target array
    for (int i = 0; i < target_count; i++)
    {
        int y = random() % max_y;
        int x = random() % max_x;
        targets_YPos[i] = y;
        targets_XPos[i] = x;
    }
    

    // Log the target positions in the log file
    for (int i = 0; i < (sizeof(targets_YPos)/4); i++)
    {
        logint(logpath, "targets_YPos", targets_YPos[i]);
    }
    logmessage(logpath, "Finished target ycoords");
    for (int i = 0; i < (sizeof(targets_XPos)/4); i++)
    {
        logint(logpath, "targets_XPos", targets_XPos[i]);
    }
    logmessage(logpath, "Finished target xcoords");


    // Encode the target positions into a message string
    bzero(socket_write_buf, sizeof(socket_write_buf));
    bzero(socket_read_buf, sizeof(socket_read_buf));
    char count[250];
    char *seperator2 = "|";
    sprintf(count, "T[%d]", target_count);
    strcat(socket_write_buf, count);
    for (int i = 0; i < target_count; i++)
    {
        char pos[400];
        sprintf(pos, "%.3f,%.3f", (float)targets_XPos[i], (float)targets_YPos[i]);
        strcat(socket_write_buf, pos);
        if (i != (target_count - 1))
        {
            strcat(socket_write_buf, seperator2);
        }
    }


    // Create a fd set for the socket
    fd_set cfds;
    struct timeval tval;
    FD_ZERO(&cfds);
    FD_SET(network_socket, &cfds);
    tval.tv_sec = 0;
    tval.tv_usec = 10000;
    int cmaxfd;
    cmaxfd = network_socket;


    // Send the target positions to the server
    n = write_and_echo(network_socket, socket_write_buf, socket_read_buf, sizeof(socket_write_buf), sizeof(socket_read_buf), "Error: Socket write", "Sent target positions", "Error: Socket read", "Read target_pos echo", "Error: Wrong echo");


    while(1)
    {
        // Log the completetion of a cycle
        logmessage(logpath, "Completed 1 cycle");


        // Reset the fd set for the socket
        FD_ZERO(&cfds);
        FD_SET(network_socket, &cfds);
        

        // Reset the socket buffers
        bzero(socket_write_buf, sizeof(socket_write_buf));
        bzero(socket_read_buf, sizeof(socket_read_buf));

        // Check for new data on the socket
        block_signal(SIGUSR1);
        int value = select((cmaxfd + 1), &cfds, NULL, NULL, &tval);
        unblock_signal(SIGUSR1);
        if (value == -1)
        {
            logerror(logpath, "Error: select", errno);
        }

        // If new data is available, read and perform the necessary actions
        if (FD_ISSET(network_socket, &cfds))
        {
            // Read the message from the server
            n = read_and_echo(network_socket, socket_read_buf, sizeof(socket_read_buf), "Error: Socket read", "Read request", "Error: Socket write", "Sent request echo");
            if (n > 0)
            {
                logstring(logpath, "Request", socket_read_buf);
            }
        }


        // If GE generate a new set of targets and send it 
        if ((strcmp(socket_read_buf, "GE"))==0)
        {
            // Generate random x and y positions for each target in a target array
            for (int i = 0; i < target_count; i++)
            {
                int y = random() % max_y;
                int x = random() % max_x;
                targets_YPos[i] = y;
                targets_XPos[i] = x;
            }


            // Encode the target positions into a message string
            bzero(socket_write_buf, sizeof(socket_write_buf));
            bzero(socket_read_buf, sizeof(socket_read_buf));
            char count[250];
            char *seperator = "|";
            sprintf(count, "T[%d]", target_count);
            strcat(socket_write_buf, count);
            for (int i = 0; i < target_count; i++)
            {
                char pos[400];
                sprintf(pos, "%.3f,%.3f", (float)targets_XPos[i], (float)targets_YPos[i]);
                strcat(socket_write_buf, pos);
                if (i != (target_count - 1))
                {
                    strcat(socket_write_buf, seperator);
                }
            }


            // Send the target positions to the server
            n = write_and_echo(network_socket, socket_write_buf, socket_read_buf, sizeof(socket_write_buf), sizeof(socket_read_buf), "Error: Socket write", "Sent target positions", "Error: Socket read", "Read target_pos echo", "Error: Wrong echo");
        }
        else if ((strcmp(socket_read_buf, "STOP"))==0)
        {
            // Send a SIGUSR2 to the watchdog to kill all the processes
            logmessage(logpath, "Received STOP message");
            block_signal(SIGUSR1);
            kill(watchdog_pid, SIGUSR1);
            unblock_signal(SIGUSR1);
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


            // Close the fd that sends the pid to the watchdog
            if((close(targets2watchfd)) == -1)
            {
                logerror(logpath, "error: targets2watchfd close", errno);
            }
            else
            {
                logmessage(logpath, "Closed targets2watchfd");
            }


            // Close the fd that receives the pid from watchdog
            if((close(watch2targetsfd)) == -1)
            {
                logerror(logpath, "error: watch2targetsfd close", errno);
            }
            else
            {
                logmessage(logpath, "Closed watch2targetsfd");
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
        req.tv_nsec = DELTA_TIME_NSEC;
        req.tv_sec = 0;
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