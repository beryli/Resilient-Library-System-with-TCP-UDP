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
#include <ctype.h>
// cpp headers
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <unordered_map>

using namespace std;

#define PORT_TCP_M "45222"
#define LOCALHOST "127.0.0.1"
#define MAXLINE 1024

struct sockaddr_in addr_serverM;
struct sockaddr_storage addr_self;
socklen_t len_self;
int sockfd_tcp;  // socket file descriptor
int myport;
char username[60], password[60];
char username_en[60], password_en[60];
char message[MAXLINE], buffer[MAXLINE];

void create_socket_tcp() {
    if ((sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("[ERROR] Client: socket creation failed\n");
        exit(1);
    }
}

void connect_tcp() {
    addr_serverM.sin_family = AF_INET;
    addr_serverM.sin_port = htons(atoi(PORT_TCP_M));
    if (connect(sockfd_tcp, (struct sockaddr*)&addr_serverM, sizeof(addr_serverM)) == -1) {
        close(sockfd_tcp);
        printf("[ERROR] Client: connect failed\n");
        exit(1);
    }
}

/**
 * Encrypt str[] to dest[]
 * Offset each character and/or digit by 5.
 * Special characters (including spaces and decimal points) will not be encrypted or changed
 */
void encrypt(char dest[], const char str[]) {
    int len = strlen(str);
    
    for (int i = 0; i < len; i++ ) {
        if (isdigit(str[i])) {
            dest[i] = str[i] < '5' ? str[i] + 5 : str[i] - 5;
        } else if (isalpha(str[i])) {
            if ((str[i] >= 'a' && str[i] <= 'u') || (str[i] >= 'A' && str[i] <= 'U')) {
                dest[i] = str[i] + 5;
            } else {
                dest[i] = str[i] - 21;
            }
        } else {
            dest[i] = str[i];
        }
    }
    dest[len] = '\0';  // Null-terminate the encrypted string
}

void get_port() {
    len_self = sizeof addr_self;
    getsockname(sockfd_tcp, (struct sockaddr*)&addr_self, &len_self);
    if (addr_self.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr_self;
        myport = ntohs(s->sin_port);
    } else { // AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr_self;
        myport = ntohs(s->sin6_port);
    }
}

/**
 * This function facilitates the process of requesting book information from the Main Server. 
 * It prompts the user to enter a book code, sends the request to the server over a TCP connection, and receives and processes the server's response.
 * The function then displays the outcome of the request, providing information about the availability of the requested book.
 * If the user has administrator rights, additional details are displayed, namely the total number of books in the system.
 */
void request_book() {
    /*** Enter a book code ***/
    memset(message, 0, sizeof(message));
    printf("Please enter book code to query: ");
    scanf("%s", message);
    /*** Send the book status equest to the Main server ***/
    send(sockfd_tcp, message, strlen(message), 0);
    if ( strcmp(username, "admin") == 0 ) {
        printf("Request sent to the Main Server with Admin rights.\n");
    } else {
        printf("%s sent the request to the Main Server.\n", username);
    }
    /*** Receive the book status response from the Main server ***/
    memset(buffer, 0, sizeof buffer);
    read(sockfd_tcp, buffer, MAXLINE-1);
    printf("Response received from the Main Server on TCP port: %d.\n", myport);
    /*** Process result ***/
    int count = atoi(buffer);
    if ( count == -1 ) {
        printf("Not able to find the book-code %s in the system.\n", message);
    } else if ( count == 0 ) {
        if ( strcmp(username, "admin") == 0 ) {
            printf("Total number of book %s available = %d\n", message, count);
        } else {
            printf("The requested book %s is NOT available in the library.\n", message);
        }
    } else {
        if ( strcmp(username, "admin") == 0 ) {
            printf("Total number of book %s available = %d\n", message, count);
        } else {
            printf("The requested book %s is available in the library.\n", message);
        }
    }
    printf("—- Start a new query —-\n");
}

/**
 * This function handles the user login process. 
 * It prompts the user to enter their username and password, encrypts the entered values, and sends them to the Main Server over a TCP connection. 
 * It then receives the authentication result from the server and displays a corresponding message. 
 * If the authentication is successful, the function enters a loop to handle book requests; otherwise, the function ends and the user can try again.
 */
void login() {
    /*** Enter, encrypt and send username and password ***/
    memset(username, 0, sizeof(username));
    printf("Please enter the username: ");
    scanf("%s", username);
    encrypt(username_en, username);
    send(sockfd_tcp, username_en, strlen(username_en), 0);
    memset(password, 0, sizeof(password));
    printf("Please enter the password: ");
    scanf("%s", password);
    encrypt(password_en, password);
    send(sockfd_tcp, password_en, strlen(password_en), 0);
    printf("%s sent an authentication request to the Main Server.\n", username);

    /*** Receive login result from Main Server ***/
    get_port();
    memset(buffer, 0, sizeof buffer);
    recv(sockfd_tcp, buffer, MAXLINE-1, 0);
    printf("%s received the result of authentication from Main Server using TCP over port %d. %s\n", username, myport, buffer);

    /*** Only enter a loop to process the book request if the authentication is successful ***/
    if ( strcmp(buffer, "Authentication is successful.") == 0 ) {
        while(1) {
            request_book();
        }
    }
}

int main() {
    create_socket_tcp();
    connect_tcp();
    printf("Client is up and running.\n");
    while(1) {
        login();
    }
    close(sockfd_tcp);
    return 0;
}