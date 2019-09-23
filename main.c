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

#define    PORT1    8002
#define    PORT2    8003


//int sockfd, connfd;
//struct sockaddr_in servaddr, cli;





//int sock=0, valread;






int main(int argc,char **argv){
    
    struct socket_connection sock_listen;
    struct socket_connection sock_send;
    
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

    close(sock_send.sockfd);
    close(sock_listen.sockfd);

    return 0;
}
