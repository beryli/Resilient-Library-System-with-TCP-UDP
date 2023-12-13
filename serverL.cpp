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

#define PORT_UDP_L "42222"
#define PORT_UDP_M "44222"
#define LOCALHOST "127.0.0.1"
#define INPUT_L "literature.txt"
#define MAXLINE 1024

int sockfd_udp;  // socket file descriptor
struct sockaddr_in addr_serverM;
socklen_t addrlen_M;
char buffer[MAXLINE];
map<std::string, int> book_status_L;

void create_socket_UDP() {
    struct addrinfo hints_udp, *res_udp;
    
    memset(&hints_udp, 0, sizeof hints_udp);
    hints_udp.ai_family = AF_INET;      // use IPv4 or IPv6, whichever
    hints_udp.ai_socktype = SOCK_DGRAM;
    hints_udp.ai_flags = AI_PASSIVE;      // fill in my IP for me

    if (getaddrinfo(LOCALHOST, PORT_UDP_L, &hints_udp, &res_udp) != 0) {     // first, load up address structs with getaddrinfo():
        perror("[ERROR] Server L: getaddrinfo failed");
        exit(1);
    }
    if ((sockfd_udp = socket(res_udp->ai_family, res_udp->ai_socktype, res_udp->ai_protocol)) == -1) {     // make a socket:
        perror("[ERROR] Server L: socket creation failed");
        exit(1);
    }
    
    if (bind(sockfd_udp, res_udp->ai_addr, res_udp->ai_addrlen) == -1) {   // bind it to the port we passed in to getaddrinfo():
        perror("[ERROR] Server L: bind socket failed");
        exit(1);
    }
    freeaddrinfo(res_udp);
}

void init_connection_serverM() {
    memset(&addr_serverM, 0, sizeof(addr_serverM));
    addr_serverM.sin_family = AF_INET; 
    addr_serverM.sin_addr.s_addr = inet_addr(LOCALHOST);
    addr_serverM.sin_port = htons(atoi(PORT_UDP_M));
    addrlen_M = sizeof(addr_serverM);
}

void read_file() {
    FILE *fptr = fopen(INPUT_L, "r");
    if (fptr == NULL) {
        printf("File %s not found", INPUT_L);
        exit(1);
    }

    book_status_L.clear();
    char myString[100];
    while(fgets(myString, 100, fptr)) {
        /*** store the information ***/
        std::string s(myString);
        std::string code = s.substr(0, s.find(','));
        int num = stoi(s.substr(s.find(" ")+1));
        book_status_L[code] = num;
        // cout  << "cpp: "<< code << " " << book_status_L[code] << endl;
    }
    fclose(fptr);
}

void check_book() {
    /*** receive book code ***/
    int n;
    memset(buffer, 0, sizeof buffer);
    if ( (n = recvfrom(sockfd_udp, buffer, MAXLINE, 0, (struct sockaddr *) &addr_serverM, &addrlen_M)) == -1) {
        perror("fail to receive data");
        exit(1);
    }
    buffer[n] = '\0'; 
    std::string msg(buffer);
    std::string username = msg.substr(0, msg.find(' '));
    std::string bookcode = msg.substr(msg.find(" ")+1);
    if ( username != "firns" ) {
        printf("Server L received %s code from the Main Server.\n", bookcode.c_str());
    } else {
        printf("Server L received an inventory status request for code %s.\n", bookcode.c_str());
    }
    
    /*** check bookcode availability ***/
    int count = -1;
    if ( book_status_L.find(bookcode) != book_status_L.end() ) {
        count = book_status_L[bookcode];
        if ( book_status_L[bookcode] > 0 && username != "firns" ) { book_status_L[bookcode]--; }
    }
    /*** Send the availability status to the Main Server ***/
    memset(buffer, 0, sizeof buffer);
    snprintf(buffer, sizeof(buffer), "%d", count);
    if (sendto(sockfd_udp, buffer, strlen(buffer) + 1, 0, (struct sockaddr *) &addr_serverM, sizeof addr_serverM) < 0) {
        perror("[ERROR] Server L: fail to send book availability status to server M via UDP");
        exit(1);
    }
    if ( username != "firns" ) {
        printf("Server L finished sending the availability status of code %s to the Main Server using UDP on port %s.\n", bookcode.c_str(), PORT_UDP_L);
    } else {
        printf("Server L finished sending the inventory status to the Main server using UDP on port %s.\n", PORT_UDP_L);
    }
}

int main() {
    create_socket_UDP();
    printf("Server L is up and running using UDP on port %s.\n", PORT_UDP_L);
    read_file();
    init_connection_serverM();
    while(1) {
        check_book();
    }
    close(sockfd_udp);
    return 0;
}