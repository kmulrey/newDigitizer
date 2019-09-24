#include <stdio.h>
#include <stdlib.h>
//#include "add.h"
//#include <iostream>
#include <sys/wait.h>
#include <sys/time.h>
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
#include <errno.h>
//#include "LS.h"
//#include "amsg.h"
//#include "scope.h"
//#include "ad_shm.h"
#define DEVFILE "/dev/scope" //!< Device for talking to the FPGA
#define DEV int32_t //!< the type of the device id is really just a 32 bit integer

DEV dev = 0;                    //!< Device id
int station_id;

#define MSG_START    0x99 //!< start of fpga message
#define MSG_END      0x66 //!< end of fpga message


/* Time stamp offsets (within 7B time field) */

#define TIME_YEAR       0
#define TIME_MON        2
#define TIME_DAY        3
#define TIME_HOUR       4
#define TIME_MIN        5
#define TIME_SEC        6


/*----------------------------------------------------------------------*/
/* Maxima / minima */

/* Maximum ADC size = 4 channels * max_samples/ch * 2 bytes/sample */
/* Maximum event size = header + ADC data + message end       */

#define DATA_MAX_SAMP   4096                       //!< Maximal trace length (samples)
#define MAX_READOUT     (70 + DATA_MAX_SAMP*8 + 2) //!< Maximal raw event size

#define MIN_MSG_LEN     6                          //!< Minimal length of scope message

/*----------------------------------------------------------------------*/
/* Message definitions */
#define ID_PARAM_GPS          0x00
#define ID_PARAM_CTRL         0x01
#define ID_PARAM_WINDOWS      0x02
#define ID_PARAM_COMRES       0x03
#define ID_PARAM_SPI          0x04
#define ID_PARAM_TRIG5        0x05
#define ID_PARAM_CH1          0x08
#define ID_PARAM_CH2          0x09
#define ID_PARAM_CH3          0x0A
#define ID_PARAM_CH4          0x0B
#define ID_PARAM_TRIG1        0x0C
#define ID_PARAM_TRIG2        0x0D
#define ID_PARAM_TRIG3        0x0E
#define ID_PARAM_TRIG4        0x0F
#define ID_PARAM_FILT11       0x10
#define ID_PARAM_FILT12       0x11
#define ID_PARAM_FILT21       0x12
#define ID_PARAM_FILT22       0x13
#define ID_PARAM_FILT31       0x14
#define ID_PARAM_FILT32       0x15
#define ID_PARAM_FILT41       0x16
#define ID_PARAM_FILT42       0x17
#define ID_PARAM_PPS          0xC4
#define ID_PARAM_EVENT        0xC0
#define ID_PARAM_ERROR        0xCE
#define ID_PARAM_NOP          0x66

/* Error Definition */
#define ERROR_BCNT 2
#define ERROR_ID   4
#define ERROR_END  6

#define PARAM_NUM_LIST 0x18     //!< Number of parameter lists for the fpga
#define PARAM_LIST_MAXSIZE 22   //!< maximal listsize 22 bytes

uint8_t shadowlist[PARAM_NUM_LIST][PARAM_LIST_MAXSIZE];  //!< all parameters to set in FPGA
uint8_t shadowlistR[PARAM_NUM_LIST][PARAM_LIST_MAXSIZE]; //!< all parameters read from FPGA
int32_t shadow_filled = 0;                               //!< the shadow list is not filled

void ls_get_station_id()
{
    FILE *fpn;
    char line[100],wrd[20];
    int n1,n2,n3,n4;
    
    station_id = -1;
    fpn = fopen("/etc/network/interfaces","r");
    if(fpn == NULL) return;
    while(line == fgets(line,199,fpn)){
        if(sscanf(line,"%s %d.%d.%d.%d",wrd,&n1,&n2,&n3,&n4) == 5){
            if(strncmp(wrd,"address",7) == 0){
                station_id = n4;
                break;
            }
        }
    }
    fclose(fpn);
    printf("Read station %d\n",station_id);
}


int scope_open()        // ok 24/7/2012
{
    printf("Trying to open !%s!\n",DEVFILE);
    //printf("before open dev\n");
    
    dev = open(DEVFILE, O_RDWR);
    // printf("after open dev\n");
    
    if (dev == -1) {
        //    if ((dev = open(DEVFILE, O_RDWR)) == -1) {
        fprintf(stderr, "Error opening scope device file %s for read/write\n", DEVFILE);
        return(-1);
    }
    
    printf("Done opening dev = %d\n",(int)dev);
    sleep(1);
    //printf("leaving scope_open\n");
    
    return(1);
}


void scope_close()
{
    if(close(dev)<0){
        perror("scope_close:");
    }
}


void scope_write(uint8_t *buf, int32_t len) // ok 24/7/2012
{
    int32_t itot=0,i;
    
    i=0;
    while (itot < len){
        if((i = write(dev,&buf[i],len-itot))< 1) {
            printf("error writing to scope: %d\n",i);
            break;
        }
        itot+=i;

        if(itot != len)
            printf("scope_write: Cannot write all data %d at once. %d bytes written\n",len,i);
    }
}
void scope_set_parameters(uint16_t *data, int to_shadow)
{
    int i;
    printf("Writing parameters:");
    for(i=0;i<data[1]/2;++i) printf(" %04x",data[i]);
    printf("\n");
    i=(data[0]>>8)&0xff;
    //if(to_shadow == 1) printf("To Shadow: %d %d\n",i,data[1]);
    //if(i<=PARAM_NUM_LIST && i> 0 && data[1]<=PARAM_LIST_MAXSIZE && data[1]>0){
    if(sizeof(data)>0){

        //if(to_shadow == 1) memcpy(shadowlist[i],data,data[1]); //new CT 20140928
        scope_write((uint8_t *)data,data[1]);
        usleep(1000);
    }
    else{
        printf("ERROR IN SETTING PARAMETERS\n");
        for(i=0;i<data[1]/2;++i) printf(" %04x",data[i]);
        printf("\n");
    }
}
/*!
 \func int scope_raw_read(unsigned char *bf, int size)
 \brief reads data from digitizer and stores it in a buffer
 \param bf pointer to location where data can be stored
 \param size number of bytes requested
 \retval number of bytes read
 */
int scope_raw_read(uint8_t *bf, int32_t size) //ok 24/7/2012
{
    int ir;
    ir =read(dev, (void *)bf, size);
    //printf("buffer size: %d\n");
    if(ir <0 && errno != EAGAIN)
        perror("scope_raw_read");
    return(ir);
}

int scope_read_error()
{
    // Code 0x99: did not get a 0x99
    // Code 0x89: got the 99 twice
    int16_t len;
    uint8_t buffer[ERROR_END+2];
    
    buffer[0]=MSG_START;
    buffer[1] =ID_PARAM_ERROR;
    scope_raw_read(&(buffer[2]),2);
    len = *(int16_t *)&(buffer[2]);
    if(len != (ERROR_END+2))
        printf("Scope_read Error length is incorrect 0x%x!\n",len);
    scope_raw_read(&(buffer[4]),ERROR_END-2);
    printf("Reading error: ");
    for(len=0;len<ERROR_END+2;len++)
        printf("0x%x ",buffer[len]);
    printf("\n");
    return(1);
}

int scope_read(int ioff)
{
    unsigned short int totlen;
    int rread,nread,ntry;
    int ir;
    unsigned char rawbuf[4]={0,0,0,0};
    
    do{                           // flush scope until start-of-message
        nread = scope_raw_read(rawbuf,1);
        // printf("flush\n");
    } while(rawbuf[0] != MSG_START && nread>0);
    
    if(nread == 0 || rawbuf[0] != MSG_START) {
        return(0);                              // no data (should never happen)
    }
    if(rawbuf[0] != MSG_START && nread>0){    // not a start of message
        printf("Not a message start %x\n",rawbuf[0]);
        return(-1);
    }
    ntry = 0;
    // do{                           // read the identifier of the message (data-type)
    nread = scope_raw_read(&(rawbuf[1]),1);
    // printf("nread: %d\n");
    if(!nread) {
        usleep(10);
        ntry++;
    }
    //}while(nread==0 &&ntry<MAXTRY);   // second word should come in within a short time
    if(nread ==0) {
        printf("Failed to read a second word\n");
        return(-2);                     // No identifier after start-of-message
    }
    printf("rawbuf[1] = %x\n",rawbuf[1]);
    
    
    if(rawbuf[1]<PARAM_NUM_LIST){
        // move the parameters in the correct shadow list.
        // TODO: they should first be compared!
        scope_raw_read((unsigned char *)(&rawbuf[3]),2);
        if(rawbuf[3] == MSG_END || rawbuf[4] == MSG_END) return(-4);
        totlen = (rawbuf[4]<<8)+rawbuf[3];
        printf("Length of param = %x\n",totlen);
        printf("rawbuf[1] = %x\n",rawbuf[1]);
        if(totlen>PARAM_LIST_MAXSIZE) return(-4);
        nread = 4;
        shadowlistR[rawbuf[1]][0] = MSG_START;
        shadowlistR[rawbuf[1]][1] = rawbuf[1];
        shadowlistR[rawbuf[1]][2] = totlen&0xff;
        shadowlistR[rawbuf[1]][3] = (totlen>>8)&0xff;
        
        
        
        //do{                             // read out the remaining parameters
        rread = scope_raw_read(&(shadowlistR[rawbuf[1]][nread]),totlen-nread);
        if(!rread) { usleep(10); ntry++; }
        else {ntry = 0; nread+=rread;}
        //}while(nread <totlen &&ntry<MAXTRY);    // until the end or timeout
        //shadowlistR[ID_PARAM_CTRL][5] |= 0x80;
    }
    
    
    else if(rawbuf[1] == ID_PARAM_PPS) {
       // ir = scope_read_pps(ioff);
        return(ir);
    }
    else if(rawbuf[1] == ID_PARAM_EVENT)
    {
        printf("----->EVENT!!!! \n");
        //return(scope_read_event(ioff));
        
    }
    else if(rawbuf[1] == ID_PARAM_ERROR){ return(scope_read_error(ioff));
    printf("ERROR Identifier = %x\n",rawbuf[1]);
        return(-3);         }                      // bad identifier read
}


void scope_main()
{
    printf("in ls_scope_main \n");
    scope_open();
    uint8_t buff[200];
    uint32_t length = sizeof(buff);
    
    printf("Getting control registers \n");
    
    unsigned short list_request=0x0C;
    //unsigned short ctrllist[4]={0xC99,0x0006,0x66,0x66};
    unsigned short ctrllist1[3]={0x0199,0x0006,0x6666};
    unsigned short ctrllist2[3]={0x0299,0x0006,0x6666};
    unsigned short ctrllist3[3]={0x0399,0x0006,0x6666};
    unsigned short ctrllist4[3]={0x0499,0x0006,0x6666};
    unsigned short ctrllist5[3]={0x0899,0x0006,0x6666};
    unsigned short ctrllist6[3]={0x0999,0x0006,0x6666};
    unsigned short ctrllist7[3]={0x0A99,0x0006,0x6666};
    unsigned short ctrllist8[3]={0x0B99,0x0006,0x6666};
    unsigned short ctrllist9[3]={0x0C99,0x0006,0x6666};
    unsigned short ctrllist10[3]={0x0D99,0x0006,0x6666};

    scope_set_parameters(ctrllist1,1);
    scope_set_parameters(ctrllist2,1);
    scope_set_parameters(ctrllist3,1);
    scope_set_parameters(ctrllist4,1);
    scope_set_parameters(ctrllist5,1);
    scope_set_parameters(ctrllist6,1);
    scope_set_parameters(ctrllist7,1);
    scope_set_parameters(ctrllist8,1);
    scope_set_parameters(ctrllist9,1);
    scope_set_parameters(ctrllist10,1);

    int ir;
    int i;
    usleep(500000);

    for(i=0; i<15;i++){
        printf("________%d_______\n",i);
        //scope_set_parameters(ctrllist,1);

        scope_read(1);
        //ir =read(dev, (void *)buff, sizeof(buff));
        //read=scope_raw_read(&(buff),length);
        //printf("from buffer: %04x  %04x  %04x  %04x\n",buff[0],buff[1],buff[2],buff[3]);
        //usleep(100000);
    }
    
    for(i=0; i<PARAM_LIST_MAXSIZE; i++){
        printf("shadow: %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x\n",shadowlistR[1][i],shadowlistR[2][i],shadowlistR[3][i],shadowlistR[4][i],shadowlistR[8][i],shadowlistR[9][i],shadowlistR[10][i],shadowlistR[11][i],shadowlistR[12][i],shadowlistR[13`1][i]); //!< all parameters read from FPGA
    }
     
    //scope_write((uint8_t *)ctrllist,sizeof(ctrllist));
    //usleep(1000);
    scope_close();

    
}

