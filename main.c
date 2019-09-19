#include <stdio.h>
#include <stdlib.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <include/comm.h>
#include <errno.h>
#include "Constants.h"
#include "FUNCTION_Unit.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#define PORT 8081
#define PORT2 8082

int ls_port;       //!<port number on which to connect to the central daq
//int32_t LS_socket = -1;   //!< main socket for accepting connections
int32_t LS_comms = -1;    //!< socket for the accepted connection
//struct sockaddr_in  LS_address; //!< structure containing address information of socket
socklen_t LS_alength;           //!< length of sending socket address
socklen_t RD_alength;           //!< length of receiving socket address
struct sockaddr_in Host_Addr ;
struct sockaddr_in serv_address;
struct hostent *Host ;
struct sockaddr_in serv_addr;

int sockfd, connfd;
struct sockaddr_in servaddr, cli;


#define MAX 80
#define SA struct sockaddr


int sock=0, valread;



fd_set sockset;           //!< socket series to be manipulated


int  Open_Socket(int PORT_NUM)
{
    printf("okay\n");
    char message[100] ={ '\0' };
    //int sockfd, connfd;
    //struct sockaddr_in servaddr, cli;
    
    // socket create and varification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
    printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
    
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("192.168.56.1");
    servaddr.sin_port = htons(PORT);
    
    return sockfd;
}

void  Close_Socket()
{
    close(sockfd);
}



void Connect_Socket(int PORT_NUM)
{
    printf("in Connect_Socket\n");
 
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
    printf("connected to the server..\n");
    
   

}


int make_server_connection(int PORT_NUM)
{
    
    printf("in make_server_connection\n");
    sockfd=Open_Socket(PORT) ;
    Connect_Socket(PORT) ;
    //func_write(sockfd);
    
    
    return(0);

}




void socket_main()
{

    printf("Opening Connection\n");
    if(make_server_connection(PORT) < 0) {  // connect to DAQ
        printf("Cannot open sockets\n");
        exit(-1);
    }

    

   
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
        //read(sockfd, buff, sizeof(buff));
        //printf("From Server : %s", buff);
       
    }
}

/*
void func_listen(int sockfd)
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
        //read(sockfd, buff, sizeof(buff));
        //printf("From Server : %s", buff);
        
    }
}
*/

int main(int argc,char **argv){
    
    
    
    printf("okay\n");

    socket_main();
    
    
    /*
    
    char message[100] ={ '\0' };
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
        
    // socket create and varification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
        
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("192.168.56.1");
    servaddr.sin_port = htons(PORT);
   
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
    printf("connected to the server..\n");
    */
    
    
    
    
    
    
 
    
    //function for chat
    func_write(sockfd);
        
    // close the socket
    Close_Socket();

    return 0;
}
