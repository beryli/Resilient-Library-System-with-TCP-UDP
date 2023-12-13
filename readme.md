# Secure Library Connect
Secure Library Connect is a comprehensive and secure Library Management System designed to streamline library operations, enhance user experience, and ensure the integrity of inventory management. The system is divided into distinct phases, each addressing specific functionalities and security measures.

## Features:
### Boot-up (Phase 1):
The system follows a specific process flow, starting with the Main Server, followed by Department Servers (Science, Literature, History), and finally, the Client.
Backend servers read corresponding input files, store information, and send book statuses to the Main Server via UDP.
On-screen messages indicate successful boot-up and data transfer.
### Login and Confirmation (Phase 2):
Users authenticate using a secure process where the client encrypts login information with a case-sensitive offset encryption scheme.
Main Server verifies credentials and responds to the client over a TCP connection.
Encryption scheme: Offset each character and/or digit by 5, excluding special characters.
### Forwarding Request to Backend Servers (Phase 3):
Clients submit book codes, and the Main Server forwards requests to the appropriate Department Server based on the code prefix (S, L, H).
TCP connections facilitate efficient communication.
### Reply (Phase 4):
Genre servers check availability, reply to the Main Server using UDP with appropriate messages ("The requested book is available," "Not available," or "Not able to find the book").
Main Server forwards the reply to the client over TCP, and on-screen messages indicate book availability.
### Inventory Management (Extra Credit):
Staff members can log in using Admin credentials to access real-time information on the total availability of books based on their respective codes.

## Project files
1. client.cpp:
    - Establish a TCP socket for communication with the main server.
    - Facilitate user and admin login, encrypting login credentials before transmitting them securely to the main server.
    - Provide functionality for requesting book status and display the results upon successful login.
2. serverM.cpp:
    - Initialize both TCP and UDP sockets for communication with client and backend servers.
    - Retrieve membership information and await login requests from clients.
    - Forward client requests to relevant backend servers, returning the book status results to the client upon receipt.
3. serverS.cpp / serverL.cpp / serverH.cpp:
    - Set up a UDP socket to establish a connection with the main server.
    - Load respective books' status information and remain receptive to requests from the main server.
    - Validate bookcode availability and communicate the results back to the main server.

## The format of all the messages exchanged
1. User Authentication:
    - The client transmits the username and password separately to the main server for authentication. No concatenation is involved.
2. Authentication Result:
    - The main server returns an authentication message for user login.
        ```
        Example 1: "Authentication failed: Username not found."
        Example 2: "Authentication failed: Password does not match."
        Example 3: "Authentication is successful."
        ```
3. Book Code Request:
    - The client requests a book code from the main server.
    - The main server forwards the message username + " " + book code to the relevant backend server.
    - This facilitates the display of admin messages if the user is an admin.
        ```
        Example: "firns S101" // admin requests book code S101
        ```
4. Server Responses:
    - All servers reply with the book status as an integer.
        ```
        -1  : not find
        0   : not available
        >=1 : available
        ```
5. File Format Standardization:
    - The newline character across all .txt files is \n\r.

## Getting Started
Run
```
make all
```
Then open 5 different terminal windows. On 4 terminals, start servers M, S, L and H using ```./serverM```, ```./serverS```, ```./serverL```, ```./serverH```, . On the other terminal, start the client using ```./client```.

Note, this was developed on Ubuntu 22.04 ARM64.