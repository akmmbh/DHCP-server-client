/*
    Compilation: gcc -o DHCPclient DHCPclient.c
    Execution  : ./DHCPclient <port_number> [eg. port_number = 10000, where port_number is the DHCP server port number]
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER "129.120.151.94"
#define BUFLEN 512

void die(char *s) {
    perror(s);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in si_other;
    int s, slen = sizeof(si_other), recv_len, portno;
    int transID, lifeTime;
    char buf[BUFLEN], message[BUFLEN];
    char yiaddr[BUFLEN] = "0.0.0.0";
    time_t t;

    system("clear");
    printf("This is DHCP client....\n\n");

    // Create UDP socket
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        die("socket");
    }

    memset((char*)&si_other, 0, sizeof(si_other));
    portno = atoi(argv[1]); // The port on which to listen for incoming data
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(portno);
    
    if (inet_aton(SERVER, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    // Begin sequence for DHCP discover
    printf("\nBegin DHCP 4-Handshake - Discover client arrives\n");
    printf("yiaddr: %s\n", yiaddr);

    // Initialize random number generator
    srand((unsigned)time(&t));
    
    // Generate a random 3-digit number for transaction ID
    transID = rand() % 900 + 100;
    printf("Transaction ID: %d\n", transID);

    // Send to the server, print error if it does not work
    if (sendto(s, &yiaddr, sizeof(yiaddr), 0, (struct sockaddr *)&si_other, slen) == -1) 
        die("sendto()");
    if (sendto(s, &transID, sizeof(transID), 0, (struct sockaddr *)&si_other, slen) == -1) 
        die("sendto()");

    // Begin Sequence for DHCP offer
    // Receive from server, print error if it does not work
    if ((recv_len = recvfrom(s, &yiaddr, sizeof(yiaddr), 0, (struct sockaddr *)&si_other, &slen)) == -1) 
        die("recvfrom()");
    if ((recv_len = recvfrom(s, &transID, sizeof(transID), 0, (struct sockaddr *)&si_other, &slen)) == -1) 
        die("recvfrom()");
    if ((recv_len = recvfrom(s, &lifeTime, sizeof(lifeTime), 0, (struct sockaddr *)&si_other, &slen)) == -1) 
        die("recvfrom()");

    printf("\n----------------- From DHCP server -----------------\n");
    printf("Assigned yiaddr is: %s\n", yiaddr);
    printf("For Transaction ID: %d\n", transID);
    printf("Assigned Lifetime: %d secs\n\n", lifeTime);

    // Begin sequence for DHCP request
    printf("Confirm yiaddr is: %s\n", yiaddr);
    printf("For Transaction ID: %d\n", transID);
    printf("With LifeTime of: %d\n\n", lifeTime);

    // Send to DHCP server
    if (sendto(s, &yiaddr, sizeof(yiaddr), 0, (struct sockaddr *)&si_other, slen) == -1) 
        die("sendto()");
    if (sendto(s, &transID, sizeof(transID), 0, (struct sockaddr *)&si_other, slen) == -1) 
        die("sendto()");
    if (sendto(s, &lifeTime, sizeof(lifeTime), 0, (struct sockaddr *)&si_other, slen) == -1) 
        die("sendto()");

    // Begin sequence for DHCP acknowledge - Receive acknowledgement from DHCP server
    if ((recv_len = recvfrom(s, &yiaddr, sizeof(yiaddr), 0, (struct sockaddr *)&si_other, &slen)) == -1) 
        die("recvfrom()");
    if ((recv_len = recvfrom(s, &transID, sizeof(transID), 0, (struct sockaddr *)&si_other, &slen)) == -1) 
        die("recvfrom()");
    if ((recv_len = recvfrom(s, &lifeTime, sizeof(lifeTime), 0, (struct sockaddr *)&si_other, &slen)) == -1) 
        die("recvfrom()");

    printf("--- ACK From DHCP server ---\n");
    printf("yiaddr is: %s\n", yiaddr);
    printf("Transaction ID: %d\n", transID);
    printf("Lifetime: %d secs\n\n", lifeTime);

    close(s);
    return 0;
}
