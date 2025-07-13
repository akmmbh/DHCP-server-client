/* 
    Compilation: gcc -o DHCPserver DHCPserver.c
    Execution  : ./DHCPserver <port_number> [eg. port_number = 5000, where port_number is the DHCP server port number]
*/

#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
 
#define BUFLEN 512  //Max length of buffer
 #define MAX_IPS 10
#define LEASE_DURATION 3600

typedef struct{
    char ip[16];
    time_t lease_start;
    struct sockaddr_in client_addr;
    int active //1 if leased ,0 if free
}LeaseEntry;
LeaseEntry leaseTable[MAX_IPS];

void initLeaseTable(){
    for(int i=0;i<MAX_IPS;i++){
        sprintf(leaseTable[i].ip,"192.168.10.%d",i+1);
        leaseTable[i].active=0;

    }
}

void checkExpiredLeases(){
    time_t now =time(NULL);
    for(int i=0;i<MAX_IPS;i++){
        if(leaseTable[i].active && difftime(now,leaseTable[i].lease_start)>=LEASE_DURATION){
            printf("[INFO] IP %s lease expired",leaseTable[i].ip);
            leaseTable[i].active=0;
        }
    }
}

LeaseEntry* getAvailableLease(struct sockaddr_in *client){
    checkExpiredLeases();


    //First loop: Reuse existing lease if the client already has one
    for(int i=0;i<MAX_IPS;i++){
        if(leaseTable[i].active &&leaseTable[i].client_addr.sin_addr.s_addr == client->sin_addr.s_addr&& leaseTable[i].client_addr.sin_port == client->sin_port ){
            return &leaseTable[i];//this client is already owns the IP
        }
    }

    //find a new unused ip
    for(int i=0;i<MAX_IPS;i++){
        if(!leaseTable[i].active){
            leaseTable[i].active=1;
            leaseTable[i].lease_start=time(NULL);
            leaseTable[i].client_addr=*client;
            return &leaseTable[i];//assign new IP
        }
    }
    return NULL;
}
void die(char *s)
{
    perror(s);
    exit(1);
}


 
int main(int argc, char *argv[])
{
     if(argc!=2){
        printf("Usage: %s <port>\n",argv[0]);
        exit(1);
    }
    struct sockaddr_in si_me, si_other;
    
    int s, i, j = 0, slen = sizeof(si_other), recv_len, portno;
   
    char buf[BUFLEN];
 
      LeaseEntry* lease;
   //init lease table

    initLeaseTable();
    //create a UDP socket
    if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        die("socket");
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
    portno = atoi(argv[1]); //The port on which to listen for incoming data
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(portno);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if(bind(s, (struct sockaddr *)&si_me, sizeof(si_me)) == -1)
        die("bind");
    
    system("clear");
    printf("...This is DHCP server...\n\n");

    //keep listening for data
    while(1)
    {
        do
        {
        

             // --- DISCOVER ---
        if ((recv_len = recvfrom(s, &buf, sizeof(buf), 0, (struct sockaddr *) &si_other, &slen)) == -1)
           die("recvfrom");
        printf("[DISCOVER] Received from client %s:%d\n",
               inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

        // --- TRANSACTION ID ---
        if ((recv_len = recvfrom(s, &transID, sizeof(transID), 0, (struct sockaddr *) &si_other, &slen)) == -1)
            die("recvfrom");

        lease = getAvailableLease(&si_other);
        if (!lease) {
            printf("[ERROR] No IPs available!\n");
            continue;
        }

        // --- OFFER ---
        printf("[OFFER] Offering IP %s to Transaction ID %d\n", lease->ip, transID);
        sendto(s, lease->ip, sizeof(lease->ip), 0, (struct sockaddr*)&si_other, slen);
        sendto(s, &transID, sizeof(transID), 0, (struct sockaddr*)&si_other, slen);
        sendto(s, &LEASE_DURATION, sizeof(LEASE_DURATION), 0, (struct sockaddr*)&si_other, slen);

        // --- REQUEST ---
        if ((recv_len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &si_other, &slen)) == -1)
           die("recvfrom");
        if ((recv_len = recvfrom(s, &transID, sizeof(transID), 0, (struct sockaddr *) &si_other, &slen)) == -1)
           die("recvfrom");
        if ((recv_len = recvfrom(s, &LEASE_DURATION, sizeof(LEASE_DURATION), 0, (struct sockaddr *) &si_other, &slen)) == -1)
           die("recvfrom");

        printf("[REQUEST] Client confirms IP %s\n", lease->ip);

        // --- ACK ---
        printf("[ACK] Confirming lease for IP %s\n", lease->ip);
        sendto(s, lease->ip, sizeof(lease->ip), 0, (struct sockaddr*)&si_other, slen);
        sendto(s, &transID, sizeof(transID), 0, (struct sockaddr*)&si_other, slen);
        sendto(s, &LEASE_DURATION, sizeof(LEASE_DURATION), 0, (struct sockaddr*)&si_other, slen);
    }
}

    close(s);
    return 0;
}
    
 
 
