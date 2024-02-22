Advanced_Robot_Programming-Assignment_3
================================

Introduction
----------------------
The following repository contains the solution to the third assignment for the "Advanced Robot Programming" course, as part of the Robotics Masters Programme at the University of Genoa, Italy. This assignment is a logical consequence of the previous one (https://github.com/Markie3110/Advanced_Robot_Programming_Assignment-2.git) with most funcional aspects being the same. The key difference here is that the project is to be implemented on two seperate systems that communicate with one another using sockets. One system acts as a server that the user interacts with and which, using the sockets, communicates with a client (the second system) that transmits the target and obstacle positions to it.


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



Overview
----------------------


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
