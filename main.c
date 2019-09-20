#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>



#define    PORT1    8002
#define    PORT2    8003


int sockfd, connfd;
struct sockaddr_in servaddr, cli;


#define MAX 80
#define SA struct sockaddr


//int sock=0, valread;

struct socket_connection
{
    int port;
    int sockfd, connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cli;
} ;




void make_socket(struct socket_connection* sock){
    // socket create and verification
    
    sock->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock->sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&sock->servaddr, sizeof(sock->servaddr));
    
    // assign IP, PORT
    sock->servaddr.sin_family = AF_INET;
    sock->servaddr.sin_addr.s_addr = inet_addr("192.168.56.1");
    sock->servaddr.sin_port = htons(sock->port);
    
}


void connect_socket(struct socket_connection* sock)
{
 
    if (connect(sock->sockfd, (SA*)&sock->servaddr, sizeof(sock->servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
    printf("connected to the server..\n");
}


void func_write(int sockfd)
{
    char buff[MAX];
    int n;
    for (;;) {
        bzero(buff, sizeof(buff));
        printf("Enter the string : ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n');
        write(sockfd, buff, sizeof(buff));

        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
        }
        bzero(buff, sizeof(buff));
    }
}
/*
void func_read(struct socket_connection* sock)
{
    char buff[MAX];
    int n;
    // infinite loop for chat
    for (int i=0; i<5; i++) {
        bzero(buff, MAX);
        // read the message from client and copy it in buffer
        read(sockfd1, buff, sizeof(buff));
        // print buffer which contains the client contents
        printf("From client: %s", buff);
        if ((strncmp(buff, "exit", 4)) == 0) {
            
            printf("Client Exit...\n");
            break;
        }
        
        bzero(buff, MAX);
        n = 0;
    }
}
*/


int main(int argc,char **argv){
    
    struct socket_connection sock_listen;
    struct socket_connection sock_send;
    
    sock_send.port=PORT1;
    sock_listen.port=PORT2;

    make_socket(&sock_send);
    connect_socket(&sock_send);
    make_socket(&sock_listen);
    connect_socket(&sock_listen);
    
    
    
    func_write(sock_send.sockfd);
    close(sock_send.sockfd);
    close(sock_listen.sockfd);

    return 0;
}
