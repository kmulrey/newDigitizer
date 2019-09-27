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
//#include "scopeFunctions.h"
#include "Scope.h"




void build_property_ctrlist(char *buff, int32_t size){
    
    printf("building control list %04x: \n",buff[1]);
    
    unsigned short ctrllist1[9]={0x0899,0x0012,0x3fa9,0x007f,0x3e80,0x0000,0x0000,0x0000,0x6666};
    //uint8_t set_param_list[PARAM_NUM_LIST][PARAM_LIST_MAXSIZE];
    
    
    
    
    
    /*
    printf("%04x: header\n",buff[0]);
    printf("%04x: type (params)\n",buff[1]);
    printf("%04x: channel\n",buff[2]);
    printf("%04x: none\n",buff[3]);
    
    printf("%04x: threshold 0\n",buff[4]);
    printf("%04x: threshold 1\n",buff[5]);
    int i= buff[4] << 8 | buff[5];
    printf("          --> %08x: \n",i);
    printf("          --> %d: \n",i);


    printf("%04x: HV 0\n",buff[6]);
    printf("%04x: HV 1\n",buff[7]);
    i= buff[6] << 8 | buff[7];
    printf("          --> %08x: \n",i);
    printf("          --> %d: \n",i);
    
    printf("%04x: trigger 0\n",buff[8]);
    printf("%04x: trigger 1\n",buff[9]);
    i= buff[8] << 8 | buff[9];
    printf("          --> %08x: \n",i);
    printf("          --> %d: \n",i);
    
    printf("%04x: pre-coin time 0\n",buff[10]);
    printf("%04x: pre-coin time 1\n",buff[11]);
    i= buff[10] << 8 | buff[11];
    printf("          --> %08x: \n",i);
    printf("          --> %d: \n",i);
    
    printf("%04x: coin time 0\n",buff[12]);
    printf("%04x: coin time 1\n",buff[13]);
    i= buff[12] << 8 | buff[13];
    printf("          --> %08x: \n",i);
    printf("          --> %d: \n",i);
    
    printf("%04x: post-coin time 0\n",buff[14]);
    printf("%04x: post-coin time 1\n",buff[15]);
    i= buff[14] << 8 | buff[15];
    printf("          --> %08x: \n",i);
    printf("          --> %d: \n",i);
     */
    
}
