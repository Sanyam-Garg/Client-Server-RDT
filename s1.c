/* TODO
    [X]. Receive data from clients and send back ACKs
    [X]. Handle receiving from two clients one after the another
    [X]. Implement dropping packets
    [X]. Do not buffer out of order packets
    [X]. Write to the file
    [X]. Output format
*/

#include<stdio.h>
#include<stdlib.h>

#include<sys/socket.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>

#include<time.h>

#define PORT 6969
#define MAXPENDING 2
#define BUFLEN 256
#define PDR 30

int lastSeqNum_1 = -1, lastSeqNum_2 = -1;

typedef struct{
    int payload_size;
    long seq_no;
    short type; // 0 means data packet, 1 means ACK packet
    char payload[BUFLEN];
}packet;

void writeToFile(FILE *fp, char payload[BUFLEN]){
    int i = 0;
    while(payload[i] != '\0'){
        fputc(payload[i], fp);
        i++;
    }
    fputc(',', fp);
    fclose(fp);
}

short drop_or_not() {
    // Seed the random number generator with the current time
    srand(time(NULL));
    
    // Generate a random integer between 1 and 100
    int random = rand() % 100 + 1;
    if(random <= PDR){
        return 1;
    }

    return 0;
}

int main(){
    // Create a socket
    int server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server_socket < 0){
        printf("Error while server socket creation\n");
        exit(0);
    }

    // Create server address info struct
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    printf("Server address assigned successfully.\n");

    // Bind to socket
    int bind_code = bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr));
    if(bind_code < 0){
        printf("Error while binding\n");
        exit(0);
    }

    printf("Binding successful.\n");

    // Listen
    int listen_code = listen(server_socket, MAXPENDING);
    if(listen_code < 0){
        printf("Error in listening\n");
        exit(0);
    }

    printf("Now listening for incoming connections\n");

    // Create client address struct for incoming connections
    struct sockaddr_in client_address1;
    int client_length = sizeof(client_address1);
    int client_socket1 = accept(server_socket, (struct sockaddr*) &client_address1, &client_length);
    if(client_socket1 < 0){
        printf("Error in client socket.\n");
        exit(0);
    }

    struct sockaddr_in client_address2;
    int client_socket2 = accept(server_socket, (struct sockaddr*) &client_address2, &client_length);
    if(client_socket2 < 0){
        printf("Error in client socket.\n");
        exit(0);
    }

    printf("Handling client 1: %s\n", inet_ntoa(client_address1.sin_addr));
    printf("Handling client 2: %s\n", inet_ntoa(client_address2.sin_addr));

    packet rcvd_pk, send_pk;

    int state = 0, drop = 0;

    // Receive and send packets to the client
    while(1){
        switch(state){
            case 0: // Wait for data from client 1

                drop = drop_or_not();
                int rcv_code1 = recv(client_socket1, &rcvd_pk, sizeof(rcvd_pk), 0);
                if(rcv_code1 < 0){
                    printf("Error in receiving packet\n");
                    exit(0);
                }

                if(!drop){

                    printf("RCVD PKT: Seq. No. = %d, Size = %d Bytes\n", rcvd_pk.seq_no, rcvd_pk.payload_size);

                    if(rcvd_pk.seq_no > lastSeqNum_1){
                        lastSeqNum_1 = rcvd_pk.seq_no;
                        // Create an output file
                        FILE *fp = fopen("output.txt", "a+");
                        if(!fp){
                            printf("Error while opening the output file.\n");
                            exit(0);
                        }
                        writeToFile(fp, rcvd_pk.payload);
                        state = 1;
                    } else{
                        state = 0;
                    }
                    

                    // Send ACK to client 1
                    memset(&send_pk.payload, 0, sizeof(send_pk.payload));
                    send_pk.payload_size = 0;
                    send_pk.seq_no = rcvd_pk.seq_no;
                    send_pk.type = 1;

                    int sent_bytes1 = send(client_socket1, &send_pk, sizeof(send_pk), 0);
                    // if(sent_bytes1 != sizeof(send_pk)){
                    //     printf("Error while sending msg to the client.\n");
                    //     exit(0);
                    // }

                    printf("SENT ACK: Seq. No. = %d\n", send_pk.seq_no);

                } else{
                    // Drop the packet, stay in the same state
                    printf("DROP PKT: Seq. No. %d\n", rcvd_pk.seq_no);
                    state = 0;
                }
                break;
            case 1: // Wait for data from client 2

                drop = drop_or_not();

                int rcv_code2 = recv(client_socket2, &rcvd_pk, sizeof(rcvd_pk), 0);
                if(rcv_code2 < 0){
                    printf("Error in receiving packet\n");
                    exit(0);
                }

                if(!drop){

                    printf("RCVD PKT: Seq. No. = %d, Size = %d Bytes\n", rcvd_pk.seq_no, rcvd_pk.payload_size);

                    if(rcvd_pk.seq_no > lastSeqNum_2){
                        lastSeqNum_2 = rcvd_pk.seq_no;
                        // Create an output file
                        FILE *fp = fopen("output.txt", "a+");
                        if(!fp){
                            printf("Error while opening the output file.\n");
                            exit(0);
                        }
                        writeToFile(fp, rcvd_pk.payload);
                        state = 0;
                    } else{
                        state = 1;
                    }
                    

                    // Send ACK to client 2
                    memset(&send_pk.payload, 0, sizeof(send_pk.payload));
                    send_pk.payload_size = 0;
                    send_pk.seq_no = rcvd_pk.seq_no;
                    send_pk.type = 1;

                    int sent_bytes2 = send(client_socket2, &send_pk, sizeof(send_pk), 0);

                    printf("SENT ACK: Seq. No. = %d\n", send_pk.seq_no);

                } else{
                    // Drop the packet, do not send ACK. Stay in the same state
                    printf("DROP PKT: Seq. No. %d\n", rcvd_pk.seq_no);
                    state = 1;
                }

                
                break;
        }
        
    }

    return 0;
}
