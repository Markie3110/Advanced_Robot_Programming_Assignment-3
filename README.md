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
