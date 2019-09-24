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
#include <FUNCTION_Sockets.h>
#include <time.h>
#include <scopeFunctions.h>


#define    PORT1    8002
#define    PORT2    8003
int dtime=2;

//int sockfd, connfd;
//struct sockaddr_in servaddr, cli;




//int sock=0, valread;






int main(int argc,char **argv){
    
    struct socket_connection sock_listen;
    struct socket_connection sock_send;
    
    time_t time1,time2 ;
    time1=time(NULL) ;
    printf("start time: %d\n",time1);
    int exit=0;
    
    sock_send.port=PORT1;
    sock_listen.port=PORT2;

    make_socket(&sock_send);
    connect_socket(&sock_send);
    make_socket(&sock_listen);
    connect_socket(&sock_listen);
    
    
    
    func_write_auto(sock_send.sockfd);
   // sleep(3000);
   // func_read(sock_listen.sockfd);
    func_read_message(sock_listen.sockfd);
    printf("received control messages...\n");
    //exit=func_read_message(sock_listen.sockfd);
    //exit=func_read_message(sock_listen.sockfd);
    //exit=func_read_message(sock_listen.sockfd);

    while(exit!=1)
    {
        exit=func_read_message(sock_listen.sockfd);
        usleep(1000) ;
        time2=time(NULL) ;
        if((time2-time1)>=dtime)
        {
          time1=time(NULL) ;
        printf("reset time\n");

         }
        //if (exit==1){break;}
    }
    
    printf("starting scope related things......\n");
    

    
    ls_get_station_id();

    scope_main();
    
    
    
    
    
    
    
    
    
    
    
    close(sock_send.sockfd);
    close(sock_listen.sockfd);

    return 0;
}
