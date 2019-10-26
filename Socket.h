#include <sys/types.h>
#include <stdlib.h>

/*
struct socket_connection
{
    int port;
    int sockfd, connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cli;
} ;*/

typedef struct{
    int port;
    int sockfd, connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cli;
} socket_connection;

int end_param;
int error_count;


void make_socket(socket_connection* sock);
void connect_socket(socket_connection* sock);
void func_write(int sockfd);
void func_write_auto(int sockfd);
void func_read(int sockfd1);
void send_dummy(int sockfd1);

int func_read_message(int sockfd1);
int func_listen(int sockfd1);
