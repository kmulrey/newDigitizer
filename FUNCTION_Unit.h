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

uint16_t list_id_ch[4]={0x099,0x0999,0x0A99,0x0B99};
uint16_t list_id_trigg[4]={0x0C99,0x0D99,0x0E99,0x0F99};

void initialize_parameter_lists(){
    
    printf("initializing parameter lists\n");
    int ch;
    
    
    
    
    readout_window_params[0]=0x0299; // header,list number
    readout_window_params[1]=0x0016; // byte count
    readout_window_params[10]=0x6666; // end

    
    
    
    for(ch=0; ch<4; ch++){
        ch_property_params[ch][0]=list_id_ch[ch];      // start, list number
        ch_property_params[ch][1]=0x16;      // byte count
        ch_property_params[ch][6]=0x00;  // PM voltage, filter
        ch_property_params[ch][7]=0x00;  // spare
        ch_property_params[ch][8]=0x6666;  // end
    
        ch_trigger_params[ch][0]=list_id_trigg[ch] ;    // start,list number
        ch_trigger_params[ch][1]=0x0012 ;    // byte count
        ch_trigger_params[ch][8]=0x6666 ;    // end

    }
}



void build_property_ctrlist(char *buff, int32_t size){
    
    printf("building control list %04x: \n",buff[1]);
    
    //unsigned short ctrllist1[9]={0x0899,0x0012,0x3fa9,0x007f,0x3e80,0x0000,0x0000,0x0000,0x6666};
    //uint8_t set_param_list[PARAM_NUM_LIST][PARAM_LIST_MAXSIZE];
    uint8_t i=buff[1];
    if(i==0x20){
        int ch=buff[2]-1;
        

        ch_property_params[ch][2]=0x00;      // gain correction
        //ch_property_params[ch][5]=0x00;      // gain correction
        ch_property_params[ch][3]=0x00;      // offset correction
        //ch_property_params[ch][7]=0x00;      // integration time
        ch_property_params[ch][4]=0x00;      // base max
        //ch_property_params[ch][9]=0x00;      // base max
        ch_property_params[ch][5]=0x00;     // base max
        //ch_property_params[ch][11]=0x00;  // base max
 

    
        ch_trigger_params[ch][2]=0x00 ;    // signal threshold T1
        //ch_trigger_params[ch][5]=0x00 ;    // signal threshold T1
        ch_trigger_params[ch][3]=0x00 ;    // noise threshold T2
        //ch_trigger_params[ch][7]=0x00 ;    // noise threshold T2
        ch_trigger_params[ch][4]=0x00 ;    // t_prev (n*5ns)
        //ch_trigger_params[ch][9]=0x00 ;    // t_per  (n*5ns)
        ch_trigger_params[ch][5]=0x00 ;    // tcmax (n*5ns)
        //ch_trigger_params[ch][11]=0x00 ;    // ncmax
        ch_trigger_params[ch][6]=0x00 ;    // ncmin
        //ch_trigger_params[ch][13]=0x00 ;    // qmax
        ch_trigger_params[ch][7]=0x00 ;    // qmin
 
        readout_window_params[4*(ch+1)+0]=0x00; // pre coin time
        //readout_window_params[4*(ch+1)+1]=0x00; // pre coin time
        readout_window_params[4*(ch+1)+1]=0x00; //post coin time
        //readout_window_params[4*(ch+1)+3]=0x00; //post coin time


        
    }
    /*
    uint8_t dig_mode_params[LEN_MODE_PARAM ];
    uint8_t readout_window_params[LEN_READOUT_PARAM];
    uint8_t ch1_property_params[LEN_CH_PROPERTY_PARAM];
    uint8_t ch2_property_params[LEN_CH_PROPERTY_PARAM];
    uint8_t ch3_property_params[LEN_CH_PROPERTY_PARAM];
    uint8_t ch4_property_params[LEN_CH_PROPERTY_PARAM];
    uint8_t ch1_trigger_params[LEN_CH_TRIGGER_PARAM];
    uint8_t ch2_trigger_params[LEN_CH_TRIGGER_PARAM];
    uint8_t ch3_trigger_params[LEN_CH_TRIGGER_PARAM];
    uint8_t ch4_trigger_params[LEN_CH_TRIGGER_PARAM];

    */
    
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
