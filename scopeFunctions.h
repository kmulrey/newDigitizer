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
#include "Scope.h"
//#include "LS.h"
//#include "amsg.h"
//#include "scope.h"
//#include "ad_shm.h"
#define DEVFILE "/dev/scope" //!< Device for talking to the FPGA
#define DEV int32_t //!< the type of the device id is really just a 32 bit integer

DEV dev = 0;                    //!< Device id
int station_id;
int nCh=4;

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
    //scope_open();
    uint8_t buff[200];
    uint32_t length = sizeof(buff);
    
    printf("Getting control registers \n");
    
    unsigned short list_request=0x0C;
  
    /*
    scope_set_parameters(readout_window_params,1);
    int i;
    for(i=0; i<nCh; i++){
        scope_set_parameters(ch_property_params[i],1);
        scope_set_parameters(ch_trigger_params[i],1);
    }
     */
    /*
    for(i=0; i<15;i++){
        printf("________%d_______\n",i);
        //scope_set_parameters(ctrllist,1);

        scope_read(1);
        //ir =read(dev, (void *)buff, sizeof(buff));
        //read=scope_raw_read(&(buff),length);
        //printf("from buffer: %04x  %04x  %04x  %04x\n",buff[0],buff[1],buff[2],buff[3]);
        //usleep(100000);
     
    }
     */
     
    /*
    for(i=0; i<PARAM_LIST_MAXSIZE; i++){
        printf("shadow: %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x\n",shadowlistR[1][i],shadowlistR[2][i],shadowlistR[3][i],shadowlistR[4][i],shadowlistR[8][i],shadowlistR[9][i],shadowlistR[10][i],shadowlistR[11][i],shadowlistR[12][i],shadowlistR[13][i]); //!< all parameters read from FPGA
    }
     */
    //scope_write((uint8_t *)ctrllist,sizeof(ctrllist));
    //usleep(1000);
   // scope_close();

    
}

