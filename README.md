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




Installation
----------------------


How to Run
----------------------


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


Known Errors
----------------------
On rare occassions, the simulator may hang or crash after passing the `make` command. In such a situation simply terminate both the server and client runs using CTRL+C, and execute the program again using the commands specified in the "How to Run" section above.
