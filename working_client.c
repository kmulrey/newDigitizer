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
#define MAX 80
#define SA struct sockaddr


int sock=0, valread;

int sockfd;


fd_set sockset;           //!< socket series to be manipulated



int  Open_Socket(int PORT_NUM)
{
    printf("in Open_Socket\n");

    memset(&serv_addr,'0',sizeof(serv_addr));
    
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if(inet_pton(AF_INET,"192.168.56.1",&serv_addr.sin_addr)<=0)
    {
        printf("\n bad address \n");
        return -1;
    }
    if ((sock=socket(AF_INET,SOCK_STREAM,0))<0)
    {
        printf("\n Socket creation error \n");
        
    }
    
    return sock ;
}


void Connect_Socket(int PORT_NUM)
{
    printf("in Connect_Socket\n");

    if(connect(sock,(struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
        printf("\n connection failed \n");
    }
    char *hello = "Hello from client";
    int counter=0;
    while(counter<10){
     send(sock , hello , strlen(hello) , 0 );
     printf("Hello message sent\n");
        counter=counter+1;
    }
}


int make_server_connection(int PORT_NUM)
{
    
    printf("in make_server_connection\n");
    sockfd=Open_Socket(PORT) ;
    Connect_Socket(PORT) ;

    
    
    return(0);

}

/*!
 \fn void ls_socket_main(int argc,char **argv)
 * \brief Handles all communication with the IP socket.
 * - Opens a connection to the central DAQ
 *  - In an infinite loop
 *     - Every 0.3 seconds
 *       - Send requested events
 *       - Check commands from the central DAQ
 *       - send data to the central DAQ
 *     - If there has not been contact for 20 seconds
 *       - reboot the PC
 *     - If there has not been contact for 13 seconds
 *       - close and reopen the socket
 *
 * \author C. Timmermans
 */
void ls_socket_main(int argc,char **argv)
{
    int i=0,ls_port=LS_PORT;
    struct timeval tprev,tnow,tcontact;
    struct timezone tzone;
    float tdif,tdifc;
    int prev_msg = 0;
    int prevgps = -1;
    uint32_t tgpsprev=0;
    
    printf("Opening Connection\n");

    if(make_server_connection(PORT) < 0) {  // connect to DAQ
        printf("Cannot open sockets\n");
        exit(-1);
    }
    
    
    // get current time
    gettimeofday(&tprev,&tzone);
    tcontact.tv_sec = tprev.tv_sec;
    tcontact.tv_usec = tprev.tv_usec;
    
    /*
    while(stop_process == 0){
        gettimeofday(&tnow,&tzone);
        tdif = (float)(tnow.tv_sec-tprev.tv_sec)+(float)( tnow.tv_usec-tprev.tv_usec)/1000000.;
        if(tdif>=0.3 || prev_msg == 1 ){                          // every 0.3 seconds, this is really only needed for phase2
            while(send_t3_event() > 0) usleep(10);
            prev_msg = 0;
            //printf("Check server data\n");
            if(check_server_data() > 0){  // check on an AERA command
                //printf("handled server data\n");
                tcontact.tv_sec = tnow.tv_sec;
                tcontact.tv_usec = tnow.tv_usec;
                prev_msg = 1;
            }
            if(send_server_data() > 0){                  // send data to AERA
                tcontact.tv_sec = tnow.tv_sec;
                tcontact.tv_usec = tnow.tv_usec;
            }
            tprev.tv_sec = tnow.tv_sec;
            tprev.tv_usec = tnow.tv_usec;
        }
        if(tgpsprev == 0) tgpsprev = tnow.tv_sec;
        if((tnow.tv_sec-tgpsprev)>20){
            if(*(shm_gps.next_read) != *(shm_gps.next_write)){
                if(*(shm_gps.next_write) == prevgps){
                    printf("There used to be a reboot due to timeout on socket\n");
                    //  system("/sbin/reboot");
                }
                prevgps = *(shm_gps.next_write);
            }
            tgpsprev = tnow.tv_sec;
        }
        tdifc = (float)(tnow.tv_sec-tcontact.tv_sec)+(float)( tnow.tv_usec-tcontact.tv_usec)/1000000.;
        if(tdifc>13.){ // no contact for 13 seconds!
            if(LS_comms >=0){
                shutdown(LS_comms,SHUT_RDWR);
                close(LS_comms);
                LS_comms = -1;
            }
            if(LS_socket >=0){
                shutdown(LS_socket,SHUT_RDWR);
                close(LS_socket);
                LS_socket = -1;
            }
            sleep(5);
            make_server_connection(ls_port);
            tcontact.tv_sec = tnow.tv_sec;
        }
        usleep(10000);
    }
    */
}





void func(int sockfd)
{
    char buff[MAX];
    int n;
    for (;;) {
        bzero(buff, sizeof(buff));
        printf("Enter the string : ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n')
        ;
        write(sockfd, buff, sizeof(buff));
        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        printf("From Server : %s", buff);
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
        }
    }
}



int main(int argc,char **argv){
    
    printf("okay\n");
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
    
    while(1) {
        printf("Enter a message: ");
        fgets(message, 100, stdin);
        send(fd, message, strlen(message), 0);
        //An extra breaking condition can be added here (to terminate the while loop)
    }
        
    // function for chat
    //func(sockfd);
        
    // close the socket
    //close(sockfd);
    
    //ls_socket_main(argc,argv);
    //UNIT *Master;

    
    //open_socket_katie();
    
    //pid_t pid = fork();

    //int counter=0;

    //while(stop_process == 0 && counter<3){
    //counter++;
    //printf("counter value is %d\n" , counter);
    
    //if((pid_socket = fork()) == 0) ls_socket_main(argc,argv);
    //sleep(1);
    //}

    return 0;
}
