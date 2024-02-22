Advanced_Robot_Programming-Assignment_3
================================

Introduction
----------------------
The following repository contains the solution to the third assignment for the "Advanced Robot Programming" course, as part of the Robotics Masters Programme at the University of Genoa, Italy. This assignment is a logical consequence of the previous one (https://github.com/Markie3110/Advanced_Robot_Programming_Assignment-2.git) with most funcional aspects being the same. The key difference here is that the project is to be implemented on two seperate computers that communicate with one another using sockets. One computer acts as a server that the user interacts with and which, using the sockets, communicates with a client (the second computer) that transmits the target and obstacle positions to it.


Table of Contents
----------------------
1. [Prerequisite Information]()
2. [Architecture]()
3. [Overview]()
4. [Installation]()
5. [How to Run]()
6. [Operational instructions]()
7. [Improvements compared to the previous assignment]()
8. [Known Errors]()


Prerequisite Information
----------------------
A document (called Protocol.pdf) describing the protocol used for client-server communication can be found in the "Supplementary Material" directory above. It describes in detail the manner in which the client and server are meant to interact with one another.


Architecture
----------------------
Shown here is the software architecture of the entire system depiciting the relationships among the processes, as well as the interprocess communications used in their interaction.
![Architecture](https://github.com/Markie3110/Advanced_Robot_Programming_Assignment-3/blob/main/Media/Architecture.png)


Overview
----------------------
As before, the entire system contains 7 processes, namely: main, server, user interface, drone, targets, obstacles and watchdog, that all work concurrently, across the server and client, to run the simulator. From a one computer perspective however, a computer may have either a combination of the server, UI and drone processes, or the targets and obstacles processes, depending on whether it is the server or client respectively. Besides the core programs, we also have two header files stored in the include folder: parameters and log that are used by the core files during runtime. A detailed description of each is given below.

### Main ###
As always, main is the parent process of the entire system, regardless of whether the run is a server or client, that is responsible for executing each and every process required by the simulator. Rather, than running all processes as before, main now requires a user input that is specified when running the simulation through the `make` command, that specifies whether a local run is a server or client. Using this input, make utilizes the `fork()` function to repeatedly fork itself, and then subsequently executes each process within the newly created child with `execvp()`. If the local run is a server, main executes the server, drone and UI processes. However, if it is the client, only the targers and obstacles processes are run. Once all the necessary processes have been created, main waits until all the created children have ended their execution, following which it itself is terminated.

### Server ###
The server is the first of the core processes to be run by the parent. As shown in the architecture, it acts as the primary communication node between not only processes within the server, but also between the processes across the server and client. The server process mantains a TCP server, using sockets, that actively listens for and accepts new clients on a particular port that is specified by the user at the time of program execution. As a socket is identified via a file descriptor (FD), the server simply utilizes the `select()` function to monitor the socket FD. If the socket FD is active, it is taken to mean that a particular client wishes to connect with the server, and the server proceeds to accept the connection, with the client's FD being stored in a local list for subsequent communicatons. The server identifies whether a client is a target or an obstacle via an identification message at the time of first contact, as specified in the protocol. Once a client is accepted, the server monitors the FD of the client in subsequent runs using the `select()` function to know when to read from the socket. As before, the server process also mantains the latest state of the simulator between the processes in the local computer, by continously checking for updates from the individual processes and then transmitting them whenever need via unnamed pipes. The polling for new data and the subsequent transmission, are all carried out using the `select()` mechanism mentioned before. The server runs in a loop with a time interval until it receives a terminate signal (`SIGTERM`) either from the watchdog, or due to a user input.

### User interface ###
The user interface is the frontend process for the entire system. It is the location where all the inputs from the user are gathered, as well as where all the visual outputs to the user are depicted. The process first creates a graphical user interface with the help of the `ncurses` library, consisting of two windows: one drone window, to depict the virtual environement the drone moves in, and an inspector window, that displays the drone's position numerically. Subsequently, the process enters a loop where in each iteration, it looks to see if the user has given any key inputs using the `wgetch()` function, following which it passes on the acquired keyvalue to the drone via the server. Given that there may be times the user does not provide any input, to ensure that the `wgetch()` function does not block each iteration of the loop indefinetely waiting for it, we also use the `wtimeout()` function, which specifies a maximum time interval `wgetch()` should wait for, at the end of which the execution is continued. Besides passing keyvalues, the UI also reads the latest drone, targets and obstacle positions from the server and depicts them as such. In the event that all targets have been collected, the UI displays a game end message. In this assignment, the UI also monitors the positions of the targets and obstacles. In the event that a particular target has been created too close to the border, the UI discards it as the drone is incapable of reaching it due to the repulsive forces. Similarly for the obstacles, if an obstacle received from the client is too close to the drone, to avoid an immensive force from acting on the drone suddenly, the UI discards it. As such, it is common to see the number of targets and obstacles being displayed by the UI be fewer than the number sent by the client.

### Drone ###
The drone is the process in which the entire dynamic behaviour of the drone has been modelled in. Running in a loop, the drone process computes the position the drone will occupy within the simulation space at a particular time, depending on the sum of the forces acting on it, which in turn is calculated based on the keypressed values specified by the user and the repulsive forces acting on it from the obstacles. Here, the drone process treats the borders as obstacles themselves, which also exert a repulsive force on the drone. The repulsive forces created by the obstacles is created by enabling a field around the obstacles and borders within which, the drone experiences an outward force. This force is exponentially dependent on the distance from the obstacle, as well as proportionally dependent on the speed of the drone. As such, greater the speed of the drone and smaller the distance between it and an obstacle, greater the force. Like in the UI, the drone also monitors the positions of the obstacles received by the client and discards those that are too close to the drone for the reasons stated above.

### Watchdog ###
The watchdog is the process that oversees the overall system behaviour by observing all the processes and terminating everything if any of them encounter a critical error. During their initialization, every process sends their pids to the watchdog using unnamed pipes, which in turn conveys its own pid to them in the same manner. Using these pids, the watchdog sends a `SIGUSR1` signal to a process, which in turn is supposed to send back a `SIGUSR2` signal if it is working properly. The watchdog waits for upto fifteen cycles, characterised by a time duration, for a response. Here, the number of cycles was increased as compared to the previous assignment, due to the slow nature of socket read/writes, during which all signals from the watchdog are blocked. If the process does not return the required signal within the required number of cycles, the watchdog takes this to mean that the process has encountered a critical error and subsequently terminates all the running processes. On the other hand, if the signal is received within the specificied time, the watchdog moves on to the next process. Once the watchdog reaches the final pid, it returns back to the first and starts over until the user terminates the system. Like the main process, the watchdog utizes the user input to distinguish if the local run is a server or client, and thus sends the signals to the appropriate processes accordingly.

### Targets ###
Targets is the process that generates a random number of goals/prizes for the drone to catch. Both the target count and position, is created by seeding the `srandom()` function using the current time, and then subsequently extracting the required random numbers using `random()`. In this iteration, targets is also one of the clients that interacts with the server on the other computer using sockets. The server address, as well as the port number, are given at the time of program execution. Once the necessary target positions have been computed, the targets process transmits them all to the server as per the protocol specified above. The process runs in a loop, continously polling for an update from the server using the `select()` function, that will either request for a new set of targets or be a command to terminate.

### Obstacles ###
Obstacles is the process that generates a random number of obstacles that will repulse the drone. Like targets, it too uses the `srandom()` and `random()` functions to randomly generate the obstacle count and positions. Similarly, like targets, it is the other client that interacts with the server. Unlike targets however, the obstacle continously generates a new set of obstacle positions within a time period that is specified by the user. This process, will continue to run until it receives a command from the server to terminate.

### Parameters ###
The parameter file contains a set of constants that are used by the processes, stored in one compact location.

### Log ###
The log parameter file defines the functions `logopen()`, `logmessage()`, `logint()`, `logdouble()`, `logstring()` and `logerror()`, that opens a logfile and logs either a message, datatype or error to it. This parameter file was created so as to simplify data logging and program debugging, where the following within the processes, thus avoiding the need to clutter the primary files with repeated code, as well as to allow the programmer to log data of choice in a single line. The log files can be found via the following path: 'Assignment_3/src/include/log/'.

Installation
----------------------
The core content of the project can be found in the folder "Assignment_3". To download the repository's contents to your local system you can do one of the following:

1. Using git from your local system<br>
To download the repo using git simply go to your terminal and go to a desired directory in which you wish to save the project in. Type the following command to clone the repository:
```bash
$ git clone "https://github.com/Markie3110/Advanced_Robot_Programming_Assignment-3.git"
```

2. Download the .zip from Github<br>
In a browser go to the repository on Github and download the .zip file available in the code dropdown box found at the top right. Unzip the file to access the contents and store the Assignment_3 folder in a directory of your choice.<br><br>

How to Run
----------------------
Before system execution, it is first necessary to move within the termial to the directory that contains the makefile.
```bash
cd Assignment_3/
```
The general command to run the system, for both the server and client, is as follows:
```bash
make TYPE={RUN TYPE} ADDRESS={SERVER ADDRESS} PORT={PORTNUMBER} SECONDS={OBJECT INTERVAL}
```
* `TYPE`: Specified whether a local run is a server or client. 'S' is given for server, 'C' for client.
* `ADDRESS`: The IPV4 address that the server is being run on. Although only utilized only by the client, a dummy value (can be 1.2.3.4) still has to be given for the server as well.
* `PORT`: The port the server and client should communicate through.
* `SECONDS`: The time interval in seconds between which the object process creates a new set of positions. Care has to be taken that the value given is not set too low (preferably 5 seconds). Like the address, despite it being used by the client only, a dummy value (can be 1) has to be given when running the server.
  
### Example ###
Given below is an example of how to run, both the server and client on a localhost (on the same computer). Do note, that two seperate but identical directories of the project have to be created for this, with one directory executing the server, and the other the client. Similarly, the commands given below should be executed from the appropriate directory.
1. Server<br>
The following creates a server on the port 9005. The values given for `ADDRESS` and `SECONDS` are ignored at the backend.
```bash
make TYPE=S ADDRESS=1.2.3.4 PORT=9005 SECONDS=1
```

2. Client <br>
The following creates a client, that connected to the server on the localhost(127.0.0.1) and port(9005), and whose objects are created every 10 seconds.
```bash
make TYPE=C ADDRESS=127.0.0.1 PORT=9005 SECONDS=10
```

Operational instructions
----------------------
To operate the drone use the following keys:
```
'q' 'w' 'e'
'a' 's' 'd'
'z' 'x' 'c'
```
The keys represent the following movements for the drone
* `q`: TOP-LEFT
* `w`: TOP
* `e`: TOP-RIGHT
* `a`: LEFT
* `s`: STOP
* `d`: RIGHT
* `z`: BOTTOM-LEFT
* `x`: BOTTOM
* `c`: BOTTOM-RIGHT
<br><br>In addition, `k` and `l` can be used to reset the drone to its starting point and shut down the entire system respectively.


Improvements compared to the previous assignment
----------------------
The following improvements were implemented as compared to the previous assignment:
* During testing, it was observed that the processes (especially the objects process in the client) were not respecting the time intervals that they were required to wait for. It was realised that the continous interruption of `usleep()` by the watchdog, effectively rendered the system call useless. This problem was not observed in prior assignments as all the processes effectively ran at the same rates that the signals were transmitted at by the watchdog, due to which the intervals were still being respected in most cases. As such `usleep()` was replaced with `nanosleep()`, that could now save the time remaining post interruption until the next iteration of a process loop had to be executed.
* Created two functions, `block_signal()` and `unblock_signal()`, that are used by all processes to block and then unblock critical sections of code from the watchdogs interrupts. Implemented after the socket read/writes were being constantly interrupted by the watchdog.
* Although not critical, it was seen to be a good practice to utilize the `volatile sig_atomic_t` datatype to denote the flag variables that were updated solely within the signal handlers, so as to prevent the compiler from storing the variables in the cache.


Known Errors
----------------------
On rare occassions, the simulator may hang or crash after passing the `make` command. In such a situation simply terminate both the server and client runs using CTRL+C, and execute the program again using the commands specified in the "How to Run" section above.
