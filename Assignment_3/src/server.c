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
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bits/sigaction.h>
#include <time.h>
#include "include/log.h"
#include "include/parameter.h"


// Declare global variables
volatile sig_atomic_t flag = 0; // Flag variable to inform process to clean memory, close links and quit 
int watchdog_pid; // Variable to store the pid of the watchdog
FILE *logfd; // File pointer that contains the file descriptor for the log file
char *logpath = "src/include/log/server.log"; // Path for the log file
int n; // Variable to store the values returned by the function read_and_echo and write_and_echos


// Declare FDs
int server2watchfd, watch2serverfd, UI2server_read, server2UI_write, drone2server_read, server2drone_write, UI_ack, drone_ack, UI2serverdone_read;
int server2UItargetY, server2UItargetX;
int server2UIobstaclesY, server2UIobstaclesX, server2droneobstaclesY, server2droneobstaclesX;
int port;


// Declare data variables 
int dronePositionX, dronePositionY; // Variables to hold drone position
int keypressedValue; // Variable to hold keypressedvalue
int targets_YPos[20], targets_XPos[20]; // Arrays to hold target positons
int obstacles_YPos[20], obstacles_XPos[20]; // Arrays to hold obstacle positions


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
    logmessage(logpath, "Received SIGTERM");
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


int* decode_positions(char *buf, int *xpos, int *ypos)
{
    /*
    A function that decodes the x and y positions from the received string message stored in buf, and copies them to the arrays xpos and ypos.
    */

    int count;
    char check[2];
    sscanf(&buf[2], "%1s", check);
    int j = 4;
    if (buf[3] !=  ']')
    {
        sscanf(&buf[2], "%2s", check);
        j = 5;
    }
    count = atoi(check);
    logint(logpath, "targetcount", count);
    char format[MAX_MSG_LEN] = "";
    for (int i = 0; i < count; i++)
    {
        bzero(format, sizeof(format));
        int p = 0;
        int d = j+16;
        for (; j < d; j++)
        {
            if (j != strlen(buf))
            {
                if (buf[j] == '|')
                {
                    j++;
                    break;
                }
                else
                {
                    format[p] = buf[j];
                }
            }
            else
            {
                break;
            }
            p++;
            
        }
        logstring(logpath, "format", format);
        float targetx, targety;
        sscanf(format, "%f,%f", &targetx, &targety);
        xpos[i] = (int)targetx;
        ypos[i] = (int)targety;
    }
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


void write_and_echo(int fd, char *writebuf, char *readbuf, int writebufsize, int readbufsize, char *message1, char *message2, char *message3, char *message4, char *message5, char *message6, char *message7)
{
    /*
    A function that writes to a socket and then monitors the echo, logging if it is true to the original or not.
    */

    // Write to the socket
    block_signal(SIGUSR1);
    n = write(fd, writebuf, writebufsize);
    unblock_signal(SIGUSR1);
    if (n < 0)
    {
        logerror(logpath, message1, errno);
    }
    else
    {
        // Wait for the echo from client
        logmessage(logpath, message2);
        bzero(readbuf, readbufsize);
        block_signal(SIGUSR1);
        n = read(fd, readbuf, readbufsize);
        unblock_signal(SIGUSR1);
        if (n < 0)
        {
            logerror(logpath, message3, errno);
        }
        else
        {
            if (strcmp(readbuf, writebuf) == 0)
            {
                logmessage(logpath, message4);
                logstring(logpath, message5, readbuf);
            }
            else
            {
                logerror(logpath, message6, errno);
                logstring(logpath, message7, readbuf);
            }
        }
    }
}


int main(int argc, char *argv[])
{
    // Parse the needed file descriptors from the arguments list
    int server2UItargetY, server2UItargetX;
    sscanf(argv[1], "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", &server2watchfd, &watch2serverfd, &UI2server_read, &server2UI_write, &drone2server_read, &server2drone_write, &UI_ack, &drone_ack, &server2UItargetY, &server2UItargetX, &server2UIobstaclesY, &server2UIobstaclesX, &server2droneobstaclesY, &server2droneobstaclesX, &port, &UI2serverdone_read);


    // Print the current working directory
    char cwd[100];
    getcwd(cwd, sizeof(cwd));
    printf("Current working directory: %s\n", cwd);


    // Create a log file and open it
    logopen(logpath);


    // Declare a signal handler to enable the flag when the SIGTERM signal is received
    struct sigaction act1;
    bzero(&act1, sizeof(act1));
    act1.sa_handler = &terminate_signal_handler;
    sigaction(SIGTERM, &act1, NULL);


    // Declare a signal handler to handle the signals received from the watchdog
    struct sigaction act2;
    bzero(&act2, sizeof(act2));
    act2.sa_handler = &watchdog_signal_handler;
    sigaction(SIGUSR1, &act2, NULL);


    // Create the server socket
    int server_socket, new_client_socket, client_socket_list[2], cmaxfd, clilen;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        logerror(logpath, "Error: Opening socket", errno);
    }
    else
    {
        logmessage(logpath, "Opened socket");
    }


    // Initialize the client_socket_list to 0
    for(int i = 0; i < 2; i++)
    {
        client_socket_list[i] = 0;
    }


    // Define the server address
    struct sockaddr_in server_address, client_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;


    // Bind the socket to our specified IP and host
    if((bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address))) < 0)
    {
        logerror(logpath, "Error: Server socket bind", errno);
    }
    else
    {
        logmessage(logpath, "Server binding successful");
    }


    // Listen for sockets
    if((listen(server_socket, 5)) < 0)
    {
        logerror(logpath, "Error: Server listen", errno);
    }
    else
    {
        logmessage(logpath, "Server listen successful");
    }


    // Send the pid to the watchdog
    int pid = getpid();
    int pid_buf[1] = {pid};
    write(server2watchfd, pid_buf, sizeof(pid_buf));
    logmessage(logpath, "sent pid to watchdog");


    // Receive the watchdog PID
    int watchdog_list[1] = {0};
    read(watch2serverfd, watchdog_list, sizeof(watchdog_list));
    watchdog_pid = watchdog_list[0];
    logmessage(logpath, "received watchdog pid");
    logint(logpath, "watchdog_pid", watchdog_pid);


    // Find the maxfd from the current set of fds
    int maxfd1 = -1;
    int fd1[3] = {UI2server_read, drone2server_read, UI2serverdone_read};
    for (int i = 0; i < 3; i++)
    {
        if (fd1[i] > maxfd1)
        {
            maxfd1 = fd1[i];
        }
    }


    // Find the maxfd from the current set of fds
    int maxfd2 = -1;
    int fd2[2] = {UI_ack, drone_ack};
    for (int i = 0; i < 2; i++)
    {
        if (fd2[i] > maxfd2)
        {
            maxfd2 = fd2[i];
        }
    }


    // Create the socket buffers
    char socket_target_write_buf[MAX_MSG_LEN];
    char socket_target_read_buf[MAX_MSG_LEN];
    char socket_obstacle_write_buf[MAX_MSG_LEN];
    char socket_obstacle_read_buf[MAX_MSG_LEN];
    char socket_read_buf[MAX_MSG_LEN];
    char socket_write_buf[MAX_MSG_LEN];


    // Create buffers for the process data pipes
    int UIread_buf[200];
    int droneread_buf[200];
    int UIwrite_buf[200];
    int dronewrite_buf[200];
    int UIack_buf[2];
    int droneack_buf[2];
    char UIdone_buf[200];


    // Create a fd_set for the sockets
    fd_set cfds;
    struct timeval ctval;
    FD_ZERO(&cfds);
    FD_SET(server_socket, &cfds);
    ctval.tv_sec = 0;
    ctval.tv_usec = 10000;
    cmaxfd = server_socket;


    // Create a fd_set for the process read fds
    fd_set rfds1;
    struct timeval tv1;
    FD_ZERO(&rfds1);
    FD_SET(UI2server_read, &rfds1);
    FD_SET(drone2server_read, &rfds1);
    FD_SET(UI2serverdone_read, &rfds1);
    tv1.tv_usec = 10000;
    tv1.tv_sec = 0;


    // Create a fd_set for the process write fds
    fd_set rfds2;
    struct timeval tv2;
    FD_ZERO(&rfds2);
    FD_SET(UI_ack, &rfds2);
    FD_SET(drone_ack, &rfds2);
    tv2.tv_usec = 10000;
    tv2.tv_sec = 0;


    // Run a loop with a time interval
    while (1)
    {
        // Reset the socket file descriptor sets
        FD_ZERO(&cfds);
        FD_SET(server_socket, &cfds);


        // Reset the socket buffers
        bzero(socket_target_write_buf, sizeof(socket_target_write_buf));
        bzero(socket_target_read_buf, sizeof(socket_target_read_buf));
        bzero(socket_obstacle_write_buf, sizeof(socket_obstacle_write_buf));
        bzero(socket_obstacle_read_buf, sizeof(socket_obstacle_read_buf));
        bzero(socket_read_buf, sizeof(socket_read_buf));
        bzero(socket_write_buf, sizeof(socket_write_buf));


        // Add client sockets to the set if a connection exists
        for (int i=0; i < 2; i++)
        {
            if(client_socket_list[i] > 0)
            {
                FD_SET(client_socket_list[i], &cfds);
            }
            if (client_socket_list[i] > cmaxfd)
            {
                cmaxfd = client_socket_list[i];
            }
        }


        // Listen for activity on the sockets
        block_signal(SIGUSR1);
        int cretval = select((cmaxfd+1), &cfds, NULL, NULL, &ctval);
        unblock_signal(SIGUSR1);
        if (cretval == -1)
        {
            logerror(logpath, "error: select", errno);
        }


        // If server socket is active, establish a new connection with a client
        if(FD_ISSET(server_socket, &cfds))
        {
            logmessage(logpath, "New client available");


            // Accept the new connection
            block_signal(SIGUSR1);
            clilen = sizeof(client_address);
            int new_client_socket = accept(server_socket, (struct sockaddr *) &client_address, &clilen);
            unblock_signal(SIGUSR1);
            if (new_client_socket < 0)
            {
                logerror(logpath, "Error: Client connect", errno);
            }
            else
            {
                logmessage(logpath, "Accepted new client");
            }


            // Read the ID message from the new client
            n = read_and_echo(new_client_socket, socket_read_buf, sizeof(socket_obstacle_read_buf), "Error: Socket ID read", "Read ID message", "Error: Target ID echo", "Echoed target ID message");
            if (n > 0)
            {
                logstring(logpath, "ID", socket_read_buf);
            }


            // Determine if the connection is the target or obstacle
            if ((strcmp(socket_read_buf, "TI"))==0)  // For target client
            {
                // Add the new socket to the socket list at position 0
                logmessage(logpath, "Added target fd");
                client_socket_list[0] = new_client_socket;
            }
            else if ((strcmp(socket_read_buf, "OI"))==0)  // For obstacle client
            {
                // Add the new socket to the socket list at position 1
                client_socket_list[1] = new_client_socket;       
            }
            else
            {
                logerror(logpath, "Illegal process", errno);
            }


            // Log the client_socket_list
            for (int i = 0; i < 2; i++)
            {
                logint(logpath, "client_socket_list", client_socket_list[i]);
            }


            // Send the window size to the client
            bzero(socket_write_buf, sizeof(socket_write_buf));
            bzero(socket_read_buf, sizeof(socket_read_buf));
            float max_x = 25.000;
            float max_y = 100.000;
            sprintf(socket_write_buf, "%.3f,%.3f", max_x, max_y);
            logstring(logpath, "socket_write_buf", socket_write_buf);
            write_and_echo(new_client_socket, socket_write_buf, socket_read_buf, sizeof(socket_write_buf), sizeof(socket_read_buf), "Error: Socket size write", "Sent window size", "Error: Socket size read", "Read size echo", "Size echo", "Error: Wrong size echo", "Wrong size echo");
        }


        // Listen for new data from the sockets
        bzero(socket_target_read_buf, sizeof(socket_target_read_buf));
        bzero(socket_obstacle_read_buf, sizeof(socket_obstacle_read_buf));
        for (int i=0; i < 2; i++)
        {
            if (FD_ISSET(client_socket_list[i], &cfds))
            {
                if (client_socket_list[i] == client_socket_list[0])
                {
                    // Read the target positions from the socket
                    n = read_and_echo(client_socket_list[i], socket_target_read_buf, sizeof(socket_target_read_buf), "Error: Target pos read", "Read target pos", "Error: Target pos echo", "Echoed target pos");


                    // Decode the target positions and store in a local variable
                    decode_positions(socket_target_read_buf, targets_XPos, targets_YPos);
                

                    // Log the target positions into the log file
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


                    // Send the target positions to the UI
                    write(server2UItargetY, targets_YPos, sizeof(targets_YPos));        
                    write(server2UItargetX, targets_XPos, sizeof(targets_XPos));

                }
                else if (client_socket_list[i] == client_socket_list[1])
                {
                    // Read the object positions from the socket
                    n = read_and_echo(client_socket_list[i], socket_obstacle_read_buf, sizeof(socket_obstacle_read_buf), "Error: Obstacle pos read", "Read obstacle pos", "Error: Obstacle pos echo", "Echoed obstacle pos");
                    if (n > 0)
                    {
                        logstring(logpath, "ID", socket_read_buf);
                    }
                    logint(logpath, "new_client_socket", new_client_socket);


                    // Decode the object positions and store in a local variable
                    decode_positions(socket_obstacle_read_buf, obstacles_XPos, obstacles_YPos);
            

                    // Log the obstacle positions into the logfile
                    for (int i = 0; i < (sizeof(obstacles_YPos)/4); i++)
                    {
                        logint(logpath, "obstacles_YPos", obstacles_YPos[i]);
                    }
                    logmessage(logpath, "Finished obstacles ycoords");
                    for (int i = 0; i < (sizeof(obstacles_XPos)/4); i++)
                    {
                        logint(logpath, "obstacles_XPos", obstacles_XPos[i]);
                    }
                    logmessage(logpath, "Finished obstacles xcoords");


                    // Check if the obstacle position matches the drone if yes, remove the obstacle from the array
                    for (int i = 0; i < (sizeof(obstacles_YPos)/4); i++)
                    {
                        if ((obstacles_YPos[i] >= (dronePositionY-2)) && (obstacles_YPos[i] <= (dronePositionY+2)) && (obstacles_XPos[i] >= (dronePositionX-2)) && (obstacles_XPos[i] <= (dronePositionX+2)))
                        {
                            obstacles_YPos[i] = DISCARD;
                            obstacles_XPos[i] = DISCARD;
                            logmessage(logpath, "Drone and obstacle coincide");
                        }
                    }

                }
            }
        }


        // Reset the buffers
        bzero(UIread_buf, sizeof(UIread_buf));
        bzero(droneread_buf, sizeof(droneread_buf));
        bzero(UIwrite_buf, sizeof(UIwrite_buf));
        bzero(dronewrite_buf, sizeof(dronewrite_buf));
        bzero(UIack_buf, sizeof(UIack_buf));
        bzero(droneack_buf, sizeof(droneack_buf));
        bzero(UIdone_buf, sizeof(UIdone_buf));


        // Reset the fd_set for the process read fds
        FD_ZERO(&rfds1);
        FD_SET(UI2server_read, &rfds1);
        FD_SET(drone2server_read, &rfds1);
        FD_SET(UI2serverdone_read, &rfds1);


        // Listen for activity on the pipes
        block_signal(SIGUSR1);
        int retval1 = select((maxfd1+1), &rfds1, NULL, NULL, &tv1);
        unblock_signal(SIGUSR1);
        if (retval1 == -1)
        {
            logerror(logpath, "error: select", errno);
        }


        // If active, read the pipes
        for (int i = 0; i < 3; i++)
        {
            if(FD_ISSET(fd1[i], &rfds1))
            {
                if (fd1[i] == UI2server_read)
                {
                    read(UI2server_read, UIread_buf, sizeof(UIread_buf));
                    keypressedValue = UIread_buf[0];
                }
                else if (fd1[i] == drone2server_read)
                {
                    read(drone2server_read, droneread_buf, sizeof(droneread_buf));
                    dronePositionX = droneread_buf[0]; 
                    dronePositionY = droneread_buf[1];
                }
                else if (fd1[i] == UI2serverdone_read)
                {
                    read(UI2serverdone_read, UIdone_buf, sizeof(UIdone_buf));
                    if (((strcmp(UIdone_buf, "GE"))==0) && (client_socket_list[0] != 0))
                    {
                        // Request the target client for a new set of target positions
                        bzero(socket_target_read_buf, sizeof(socket_target_read_buf));
                        bzero(socket_target_write_buf, sizeof(socket_target_write_buf));
                        char *message2 = "GE";
                        sprintf(socket_target_write_buf, "%s", message2);
                        logstring(logpath, "GE message", socket_target_write_buf);
                        write_and_echo(client_socket_list[0], socket_target_write_buf, socket_target_read_buf, sizeof(socket_target_write_buf), sizeof(socket_target_read_buf), "Error: GE write", "Sent GE message", "Error: GE read", "Read GE echo", "GE echo", "Error: Wrong echo", "Wrong GE echo");
                    }
                }
            }
        }
        logint(logpath, "keypressedValue", keypressedValue);
        logint(logpath, "dronePositionY", dronePositionY);
        logint(logpath, "dronePositionX", dronePositionX);


        // Reset the fd_set for the process write fds
        FD_ZERO(&rfds2);
        FD_SET(UI_ack, &rfds2);
        FD_SET(drone_ack, &rfds2);


        // Listen for requests on the pipes
        block_signal(SIGUSR1);
        int retval2 = select((maxfd2+1), &rfds2, NULL, NULL, &tv2);
        unblock_signal(SIGUSR1);
        if (retval2 == -1)
        {
            logerror(logpath, "error: select", errno);
        }


        // If active, write the latest data on the pipes
        for (int i = 0; i < 4; i++)
        {
            if(FD_ISSET(fd2[i], &rfds2))
            {
                if (fd2[i] == UI_ack)
                {
                    read(UI_ack, UIack_buf, sizeof(UIack_buf));
                    if(UIack_buf[0] == ACK_INT)
                    {
                        UIwrite_buf[0] = dronePositionX; 
                        UIwrite_buf[1] = dronePositionY;
                        write(server2UI_write, UIwrite_buf, sizeof(UIwrite_buf));
                        write(server2UIobstaclesY, obstacles_YPos, sizeof(obstacles_YPos));        
                        write(server2UIobstaclesX, obstacles_XPos, sizeof(obstacles_XPos));
                    }
                }
                else if (fd2[i] == drone_ack)
                {
                    read(drone_ack, droneack_buf, sizeof(droneack_buf));
                    if(droneack_buf[0] == ACK_INT)
                    {
                        dronewrite_buf[0] = keypressedValue;
                        write(server2drone_write, dronewrite_buf, sizeof(dronewrite_buf));
                        write(server2droneobstaclesY, obstacles_YPos, sizeof(obstacles_YPos));        
                        write(server2droneobstaclesX, obstacles_XPos, sizeof(obstacles_XPos));
                    }
                }
            }
        }


        // Block SIGTERM when reading from the flag variable
        block_signal(SIGTERM);
        // If the flag is set, close all links and pipes and terminate the process 
        if (flag == 1)
        {
            logmessage(logpath, "Closing all links and pipes");


            // Send a STOP signal to the target telling it to shutdown
            bzero(socket_target_read_buf, sizeof(socket_target_read_buf));
            bzero(socket_target_write_buf, sizeof(socket_target_write_buf));
            char *message3 = "STOP";
            sprintf(socket_target_write_buf, "%s", message3);
            logstring(logpath, "STOP message", socket_target_write_buf);
            write_and_echo(client_socket_list[0], socket_target_write_buf, socket_target_read_buf, sizeof(socket_target_write_buf), sizeof(socket_target_read_buf), "Error: Socket STOP write", "Sent STOP message", "Error: Target STOP echo", "Read target echo", "Target echo", "Error: Wrong target echo", "Wrong STOP echo");


            // Send a STOP signal to the obstacle telling it to shutdown
            bzero(socket_obstacle_read_buf, sizeof(socket_obstacle_read_buf));
            bzero(socket_obstacle_write_buf, sizeof(socket_obstacle_write_buf));
            sprintf(socket_obstacle_write_buf, "%s", message3);
            logstring(logpath, "STOP message", socket_obstacle_write_buf);
            write_and_echo(client_socket_list[1], socket_obstacle_write_buf, socket_obstacle_read_buf, sizeof(socket_obstacle_write_buf), sizeof(socket_obstacle_read_buf), "Error: Socket STOP write", "Sent STOP message", "Error: Obstacle STOP echo", "Read obstacle echo", "Obstacle echo", "Error: Wrong obstacle echo", "Wrong STOP echo");


            // Close the server socket
            if((close(server_socket)) == -1)
            {
                logerror(logpath, "error: server_socket close", errno);
            }
            else
            {
                logmessage(logpath, "Closed server_socket");
            }

            // Close the target client socket
            if((close(client_socket_list[0])) == -1)
            {
                logerror(logpath, "error: target close", errno);
            }
            else
            {
                logmessage(logpath, "Closed target");
            }


            // Close the obstacle client socket
            if((close(client_socket_list[1])) == -1)
            {
                logerror(logpath, "error: obstacle close", errno);
            }
            else
            {
                logmessage(logpath, "Closed obstacle");
            }


            // Close the fd that receives pid from the watchdog
            if((close(watch2serverfd)) == -1)
            {
                logerror(logpath, "error: watch2serverfd close", errno);
            }
            else
            {
                logmessage(logpath, "Closed watch2serverfd");
            }

            // Close the fd that sends pid to the watchdog
            if((close(server2watchfd)) == -1)
            {
                logerror(logpath, "error: server2watchfd close", errno);
            }
            else
            {
                logmessage(logpath, "Closed server2watchfd");
            }


            // Close the fd that sends data from the UI to the server
            if((close(UI2server_read)) == -1)
            {
                logerror(logpath, "error: UI2server_read close", errno);
            }
            else
            {
                logmessage(logpath, "Closed UI2server_read");
            }


            // Close the fd that sends data from the server to the UI
            if((close(server2UI_write)) == -1)
            {
                logerror(logpath, "error: server2UI_write close", errno);
            }
            else
            {
                logmessage(logpath, "Closed server2UI_write");
            }


            // Close the fd that sendsdata from the drone to the server
            if((close(drone2server_read)) == -1)
            {
                logerror(logpath, "error: drone2server_read close", errno);
            }
            else
            {
                logmessage(logpath, "Closed drone2server_read");
            }


            // Close the fd that sends data from the server to the drone
            if((close(server2drone_write)) == -1)
            {
                logerror(logpath, "error: server2drone_write close", errno);
            }
            else
            {
                logmessage(logpath, "Closed server2drone_write");
            }


            // Close the fd that sends a request from the UI to the server for an update
            if((close(UI_ack)) == -1)
            {
                logerror(logpath, "error: UI_ack close", errno);
            }
            else
            {
                logmessage(logpath, "Closed UI_ack");
            }


            // Close the fd that sends a request from the drone to the server for an update
            if((close(drone_ack)) == -1)
            {
                logerror(logpath, "error: drone_ack close", errno);
            }
            else
            {
                logmessage(logpath, "Closed drone_ack");
            }


            // Close the fd that sends targets Y position from server to UI
            if((close(server2UItargetY)) == -1)
            {
                logerror(logpath, "error: server2UItargetY close", errno);
            }
            else
            {
                logmessage(logpath, "Closed server2UItargetY");
            }


            // Close the fd that sends targets X position from server to UI
            if((close(server2UItargetX)) == -1)
            {
                logerror(logpath, "error: server2UItargetX close", errno);
            }
            else
            {
                logmessage(logpath, "Closed server2UItargetX");
            }


            // Close the fd that sends obstacles Y position from server to UI
            if((close(server2UIobstaclesY)) == -1)
            {
                logerror(logpath, "error: server2UIobstaclesY close", errno);
            }
            else
            {
                logmessage(logpath, "Closed server2UIobstaclesY");
            }


            // Close the fd that sends obstacles X position from server to UI
            if((close(server2UIobstaclesX)) == -1)
            {
                logerror(logpath, "error: server2UIobstaclesX close", errno);
            }
            else
            {
                logmessage(logpath, "Closed server2UIobstaclesX");
            }


            // Close the fd that sends obstacles Y position from server to drone
            if((close(server2droneobstaclesY)) == -1)
            {
                logerror(logpath, "error: server2droneobstaclesY close", errno);
            }
            else
            {
                logmessage(logpath, "Closed server2droneobstaclesY");
            }


            // Close the fd that sends obstacles X position from server to drone
            if((close(server2droneobstaclesX)) == -1)
            {
                logerror(logpath, "error: server2droneobstaclesX close", errno);
            }
            else
            {
                logmessage(logpath, "Closed server2droneobstaclesX");
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
        // Unblock the signal once read from the flag variable
        unblock_signal(SIGTERM);


        // Add the wait interval for one cycle
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
        logmessage(logpath, "Server performed 1 cycle");
        
    }
    
    return 0;
}