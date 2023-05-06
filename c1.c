/* TODO:
    [X]. Read the file input
    [X]. Create Packet struct
    [X]. Send and receive data from the server
    [X]. Implement dropping packets
    [X]. Implement timeout of 2 seconds
    [X]. Output format
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<sys/socket.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>

#include<time.h>
#include<sys/time.h>

#define PORT 6969
#define BUFLEN 256

int num_of_names = 0;

typedef struct{
    int payload_size;
    long seq_no;
    short type; // 0 means data packet, 1 means ACK packet
    char payload[BUFLEN];
}packet;

packet * getNextPacket(FILE *fp){
    char c;
    static char name[BUFLEN] = {0};

    c = fgetc(fp);

    if(c == EOF){
        return NULL;
    }

    packet *pk = (packet *) (malloc(sizeof(packet)));
    pk->seq_no = ftell(fp) - num_of_names;

    int i = 0;
    while(c != ',' && c != '.'){
        name[i] = c;
        i++;
        c = fgetc(fp);
    }
    
    name[i] = '\0';
    num_of_names++;

    
    strcpy(pk->payload, name);
    pk->payload_size = i;
    pk->type = 0;
    

    return pk;
}

int main(){
    printf("This is client 1\n");

    FILE *fp = fopen("name.txt", "r");
    if(!fp){
        printf("File cannot be opened.\n");
        return 1;
    }

    // Create a socket
    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock < 0){
        printf("Error in opening a socket");
        exit(0);
    }

    printf("Client socket created successfully\n");

    // Create server address info struct
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Set a 2 second timeout on the rcv calls.
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    // Establish a connection to the server
    int conn = connect(sock, (struct sockaddr*) &server_addr, sizeof(server_addr));
    if(conn < 0){
        printf("Error while establishing connection to the server\n");
        exit(0);
    }

    printf("Connection to server established\n");

    packet send_pk, rcvd_pk;

    // Send data to the server
    packet *pk = getNextPacket(fp);
    while(pk != NULL){
        strcpy(send_pk.payload, pk->payload);
        send_pk.payload_size = pk->payload_size;
        send_pk.type = pk->type;
        send_pk.seq_no = pk->seq_no;

        free(pk);

        int sent_bytes = send(sock, &send_pk, sizeof(send_pk), 0);
        if(sent_bytes != sizeof(send_pk)){
            printf("Error while sending the message\n");
            exit(0);
        }
        printf("SND PKT: Seq. No. = %d, Size = %d Bytes\n", send_pk.seq_no, send_pk.payload_size);

        int rcvd_bytes = recv(sock, &rcvd_pk, sizeof(rcvd_pk), 0);
        while(rcvd_bytes < 0){
            // Could be some error, but since the code is working, considering this as a timeout.
            // Retransmit packet
            int sent_bytes = send(sock, &send_pk, sizeof(send_pk), 0);
            if(sent_bytes != sizeof(send_pk)){
                printf("Error while sending the message\n");
                exit(0);
            }
            printf("RE-TRANSMIT PKT: Seq. No. = %d, Size = %d Bytes\n", send_pk.seq_no, send_pk.payload_size);
            
            rcvd_bytes = recv(sock, &rcvd_pk, sizeof(rcvd_pk), 0);
        }

        printf("RCVD ACK: Seq. No. = %d\n", rcvd_pk.seq_no);

        pk = getNextPacket(fp);
        
    }
    
    
    return 0;
}