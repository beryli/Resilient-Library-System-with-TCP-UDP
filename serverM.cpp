// c headers
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
// cpp headers
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <unordered_map>
#include <utility>

using namespace std;

#define PORT_UDP "44222"
#define PORT_TCP "45222"
#define PORT_S "41222"
#define PORT_L "42222"
#define PORT_H "43222"
#define LOCALHOST "127.0.0.1"
#define INPUT_M "member.txt"
#define MAXLINE 1024
#define BACKLOG 10   // how many pending connections queue will hold

struct sockaddr_in addr_serverS, addr_serverL, addr_serverH; // When AWS works as a client
socklen_t addrlen_S, addrlen_L, addrlen_H;
struct sockaddr_in addr_client; // connector's address information
socklen_t addrlen_client;
char buffer[MAXLINE];
char bufferS[MAXLINE]; // Write result returned from server S
char buffer_client[MAXLINE]; // Write result returned from client
char buffer_self[MAXLINE]; // buffer used for sending data to others
std::string username_en, password_en;
int sockfd_udp;  // socket file descriptor
int sockfd_tcp;  // socket file descriptor
int sockfd_tcp_client;  // socket file descriptor
map<std::string, int> book_status_M;
map<std::string, std::string> members; // hold username and hashed passwords

void init_connection_serverS() {
    memset(&addr_serverS, 0, sizeof(addr_serverS));
    addr_serverS.sin_family = AF_INET;
    addr_serverS.sin_port = htons(atoi(PORT_S));
    addr_serverS.sin_addr.s_addr = inet_addr(LOCALHOST);
    addrlen_S = sizeof(addr_serverS);
}

void init_connection_serverL() {
    memset(&addr_serverL, 0, sizeof(addr_serverL));
    addr_serverL.sin_family = AF_INET;
    addr_serverL.sin_port = htons(atoi(PORT_L));
    addr_serverL.sin_addr.s_addr = inet_addr(LOCALHOST);
    addrlen_L = sizeof(addr_serverL);
}

void init_connection_serverH() {
    memset(&addr_serverH, 0, sizeof(addr_serverH));
    addr_serverH.sin_family = AF_INET;
    addr_serverH.sin_port = htons(atoi(PORT_H));
    addr_serverH.sin_addr.s_addr = inet_addr(LOCALHOST);
    addrlen_H = sizeof(addr_serverH);
}

void create_socket_TCP() {
    int opt = 1;
    struct addrinfo hints_tcp, *res_tcp;
    
    memset(&hints_tcp, 0, sizeof hints_tcp);
    hints_tcp.ai_family = AF_INET;      // use IPv4 or IPv6, whichever
    hints_tcp.ai_socktype = SOCK_STREAM;
    hints_tcp.ai_flags = AI_PASSIVE;      // fill in my IP for me

    if (getaddrinfo(NULL, PORT_TCP, &hints_tcp, &res_tcp) != 0) {     // first, load up address structs with getaddrinfo():
        perror("[ERROR] Server M: TCP - getaddrinfo failed");
        exit(1);
    }
    if ((sockfd_tcp = socket(res_tcp->ai_family, res_tcp->ai_socktype, res_tcp->ai_protocol)) < 0) {
        perror("[ERROR] Server M: TCP - socket creation failed");
        exit(1);
    }
    if (setsockopt(sockfd_tcp, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0) {
        perror("[ERROR] Server M: TCP - setsockopt failed");
        exit(1);
    }
    if (bind(sockfd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen) == -1) {   // bind it to the port we passed in to getaddrinfo():
        perror("[ERROR] Server M: TCP - bind socket failed");
        exit(1);
    }
    freeaddrinfo(res_tcp);
    if (listen(sockfd_tcp, BACKLOG) == -1) {
        perror("[ERROR] Server M: listen failed");
        exit(1);
    }
}

void create_socket_UDP() {
    struct addrinfo hints_udp, *res_udp;
    
    memset(&hints_udp, 0, sizeof hints_udp);
    hints_udp.ai_family = AF_UNSPEC;      // use IPv4 or IPv6, whichever
    hints_udp.ai_socktype = SOCK_DGRAM;
    hints_udp.ai_flags = AI_PASSIVE;      // fill in my IP for me

    getaddrinfo(NULL, PORT_UDP, &hints_udp, &res_udp);    // first, load up address structs with getaddrinfo():
    sockfd_udp = socket(res_udp->ai_family, res_udp->ai_socktype, res_udp->ai_protocol);    // make a socket:
    bind(sockfd_udp, res_udp->ai_addr, res_udp->ai_addrlen);   // bind it to the port we passed in to getaddrinfo():
    freeaddrinfo(res_udp);
}

void load_users() {
    FILE *fptr = fopen(INPUT_M, "r");
    if (fptr == NULL) {
        printf("File %s not found", INPUT_M);
        exit(1);
    }
    /*** store membership information ***/
    char myString[100];
    while(fgets(myString, 100, fptr)) {
        size_t newline_pos = strcspn(myString, "\n\r");
        myString[newline_pos] = 0;
        std::string s(myString);
        std::string username = s.substr(0, s.find(','));
        std::string password = s.substr(s.find(" ")+1);
        members[username] = password;
    }
    fclose(fptr);
}

void accept_connection() {
    addrlen_client = sizeof(addr_client);
    if ((sockfd_tcp_client = accept(sockfd_tcp, (struct sockaddr *)&addr_client, &addrlen_client)) == -1) {
        perror("[ERROR] Server M: accept failed");
        return ;
    }
}

/**
 * This function manages book requests from a client by coordinating with backend servers (S, L, H) using a UDP connection. 
 * The process involves receiving a book request, forwarding it to the appropriate backend server, processing the result, and sending the status back to the client over TCP.
 */
void manage_books() {
    int n;
    /*** Receive the book request from client ***/
    memset(buffer_client, 0, sizeof buffer_client);
    n = recv(sockfd_tcp_client, buffer_client, MAXLINE-1, 0);
    if ( n <= 0 ) { return; }
    printf("Main Server received the book request from client using TCP over port %s.\n", PORT_TCP);
    /*** Forward the book request to backend server ***/
    memset(buffer, 0, sizeof buffer);
    strcpy(buffer, username_en.c_str());
    strcat(buffer, " ");
    strcat(buffer, buffer_client);
    int count = -1;
    if ( buffer_client[0] == 'S') {
        if (sendto(sockfd_udp, buffer, sizeof buffer, 0, (struct sockaddr *) &addr_serverS, sizeof addr_serverS) < 0) {
            perror("[ERROR] Server M: fail to send book statuses to server S via UDP");
            exit(1);
        }
        memset(buffer, 0, sizeof buffer);
        if ( (n = recvfrom(sockfd_udp, buffer, MAXLINE, 0, (struct sockaddr *) &addr_serverS, &addrlen_S)) == -1) {
            perror("fail to receive data");
            exit(1);
        }
        count = atoi(buffer);
    } else if ( buffer_client[0] == 'L') {
        if (sendto(sockfd_udp, buffer, sizeof buffer, 0, (struct sockaddr *) &addr_serverL, sizeof addr_serverL) < 0) {
            perror("[ERROR] Server M: fail to send book statuses to server L via UDP");
            exit(1);
        }
        memset(buffer, 0, sizeof buffer);
        if ( (n = recvfrom(sockfd_udp, buffer, MAXLINE, 0, (struct sockaddr *) &addr_serverL, &addrlen_L)) == -1) {
            perror("fail to receive data");
            exit(1);
        }
        count = atoi(buffer);
    } else if ( buffer_client[0] == 'H') {
        if (sendto(sockfd_udp, buffer, sizeof buffer, 0, (struct sockaddr *) &addr_serverH, sizeof addr_serverH) < 0) {
            perror("[ERROR] Server M: fail to send book statuses to server H via UDP");
            exit(1);
        }
        memset(buffer, 0, sizeof buffer);
        if ( (n = recvfrom(sockfd_udp, buffer, MAXLINE, 0, (struct sockaddr *) &addr_serverH, &addrlen_H)) == -1) {
            perror("fail to receive data");
            exit(1);
        }
        count = atoi(buffer);
    } 
    /*** Process the book status result ***/
    if ( count == -1 ) {
        printf("Did not find %s in the book code list.\n", buffer_client);
    } else {
        printf("Found %s located at Server %c. Send to Server %c.\n", buffer_client, buffer_client[0], buffer_client[0]);
        printf("Main Server received from server %c the book status result using UDP over port %s:\n", buffer_client[0], PORT_UDP);
        printf("Number of books %s available is: %d\n", buffer_client, count);
    }
    /*** Send the book status result to the client ***/
    memset(buffer_self, 0, sizeof buffer_self);
    strcpy(buffer_self, buffer);
    send(sockfd_tcp_client, buffer_self, strlen(buffer_self), 0);
    printf("Main Server sent the book status to the client.\n");
}

/**
 * This function handles the authentication process between the Main Server and a client over a TCP connection. 
 * It receives the encrypted username and password from the client, validates the credentials, and sends the authentication result back to the client.
 */
void authenticate() {
    /*** Receive username and password from client ***/
    int m, n;
    memset(buffer_client, 0, sizeof buffer_client);
    m = recv(sockfd_tcp_client, buffer_client, MAXLINE-1, 0);
    username_en = std::string(buffer_client);
    memset(buffer_client, 0, sizeof buffer_client);
    n = recv(sockfd_tcp_client, buffer_client, MAXLINE-1, 0);
    password_en = std::string(buffer_client);
    if ( m <= 0 && n <= 0 ) { return; }
    printf("Main Server received the username and password from the client using TCP over port %s.\n", PORT_TCP);

    /*** Send the authentication request result to client over a TCP connection ***/
    memset(buffer_self, 0, sizeof buffer_self);
    if ( members.find(username_en) == members.end() ) {
        cout << username_en + " is not registered. Send a reply to the client.\n";
        strcpy(buffer_self, "Authentication failed: Username not found.");
        send(sockfd_tcp_client, buffer_self, strlen(buffer_self), 0);
    } else if ( members[username_en] != password_en ) {
        cout << "Password " + password_en + " does not match the username. Send a reply to the client.\n";
        strcpy(buffer_self, "Authentication failed: Password does not match.");
        send(sockfd_tcp_client, buffer_self, strlen(buffer_self), 0);
    } else {
        cout << "Password " + password_en + " matches the username. Send a reply to the client.\n";
        strcpy(buffer_self, "Authentication is successful.");
        send(sockfd_tcp_client, buffer_self, strlen(buffer_self), 0);
        while(1) {
            manage_books();
        }
    }
}

int main() {
    create_socket_TCP();
    create_socket_UDP();
    init_connection_serverS();
    init_connection_serverL();
    init_connection_serverH();
    printf("Main Server is up and running.\n");
    load_users();
    printf("Main Server loaded the member list.\n");
    /*** Server is operating ***/
    while(1) {
        accept_connection();
        if (!fork()) { 				// The child process returns 0
            close(sockfd_tcp); 		// The child process does not need to wait for the connection, only the parent process does
            while(1) {
                authenticate();
            }
        }
        close(sockfd_tcp_client);
    }
    close(sockfd_udp);
    close(sockfd_tcp);
    return 0;
}