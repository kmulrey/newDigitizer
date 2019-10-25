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
#include <time.h>
//#include <scopeFunctions.h>
#include "ad_shm.h"
#include "Scope.h"
#include "Socket.h"

#define    PORT1    8002
#define    PORT2    8003
int dtime=2;





GPS_DATA *gpsbuf; //!< buffer to hold GPS information
shm_struct shm_gps; //!< shared memory containing all GPS info, including read/write pointers

//int sockfd, connfd;
//struct sockaddr_in servaddr, cli;






//int sock=0, valread;






int main(int argc,char **argv){
    
    struct socket_connection sock_listen;
    struct socket_connection sock_send;
    
    
    if(ad_shm_create(&shm_gps,GPSSIZE,sizeof(GPS_DATA)/sizeof(uint16_t)) <0){ //ad_shm_create is in shorts!
        printf("Cannot create GPS shared memory !!\n");
        exit(-1);
    }
    printf("Created GPS shared memory \n");
    
    *(shm_gps.next_read) = 0;
    *(shm_gps.next_write) = 0;
    gpsbuf = (GPS_DATA *) shm_gps.Ubuf;
    
    
    
    
    
    
    
    
    
    
    
    time_t time1,time2 ;
    time1=time(NULL) ;
    printf("start time: %d\n",time1);
    int exit=0;
    
    initialize_parameter_lists();
    //printf("checking initialization: %04x\n",ch_property_params[0][0]);
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
    //printf("received control messages...\n");
    func_read_message(sock_listen.sockfd);
    //printf("received control messages...\n");
    func_read_message(sock_listen.sockfd);
    //printf("received control messages...\n");
    func_read_message(sock_listen.sockfd);
    //printf("received control messages...\n");
    func_read_message(sock_listen.sockfd);
    //printf("received control messages...\n");
    
    int i;
    
    printf("mode parameters: {");
    for(i=0;i<LEN_MODE_PARAM; i++){
        printf("%x  ",dig_mode_params[i]);
    }
    printf("}\n");

    printf("readout window: {");
    for(i=0;i<LEN_READOUT_PARAM; i++){
        printf("%x  ",readout_window_params[i]);
    }
    printf("}\n");
    
    printf("ch1 property params: {");
    for(i=0;i<LEN_CH_PROPERTY_PARAM; i++){
        printf("%x  ",ch_property_params[0][i]);
    }
    printf("}\n");
    
    printf("ch2 property params: {");
    for(i=0;i<LEN_CH_PROPERTY_PARAM; i++){
        printf("%x  ",ch_property_params[1][i]);
    }
    printf("}\n");
    
    printf("ch3 property params: {");
    for(i=0;i<LEN_CH_PROPERTY_PARAM; i++){
        printf("%x  ",ch_property_params[2][i]);
    }
    printf("}\n");
    
    printf("ch4 property params: {");
    for(i=0;i<LEN_CH_PROPERTY_PARAM; i++){
        printf("%x  ",ch_property_params[3][i]);
    }
    printf("}\n");
    
    printf("ch1 trigger params: {");
    for(i=0;i<LEN_CH_TRIGGER_PARAM; i++){
        printf("%x  ",ch_trigger_params[0][i]);
    }
    printf("}\n");
    printf("ch2 trigger params: {");
    for(i=0;i<LEN_CH_TRIGGER_PARAM; i++){
        printf("%x  ",ch_trigger_params[1][i]);
    }
    printf("}\n");
    printf("ch3 trigger params: {");
    for(i=0;i<LEN_CH_TRIGGER_PARAM; i++){
        printf("%x  ",ch_trigger_params[2][i]);
    }
    printf("}\n");
    printf("ch4 trigger params: {");
    for(i=0;i<LEN_CH_TRIGGER_PARAM; i++){
        printf("%x  ",ch_trigger_params[3][i]);
    }
    printf("}\n");
    

    
    
    
    
    /*
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
    */
    printf("starting scope related things......\n");
    

    
    ls_get_station_id();

    scope_main();
    
    

    
    
    
    
    close(sock_send.sockfd);
    close(sock_listen.sockfd);
    ad_shm_delete(&shm_gps);


    return 0;
}
