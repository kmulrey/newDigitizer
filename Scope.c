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
#include "ad_shm.h"


#define DEVFILE "/dev/scope" //!< Device for talking to the FPGA
#define DEV int32_t //!< the type of the device id is really just a 32 bit integer

DEV dev = 0;                    //!< Device id
int station_id;
int nCh=4;


#define MAXTRY 50               //!< maximal number of loops to complete reading from the FPGA
#define UPDATESEC 100           //!< time interval between succesive rate checks. Only used in dynamic monitoring of rate.
#ifdef STAGE1
#define THRESMAX 3000             // radio threshold
#define THRESMIN 400              // radio threshold
#else
#define THRESMAX 1000             //!< maximal radio threshold. Only used in dynamic monitoring of rate.
#define THRESMIN 20               //!< minimal radio threshold. Only used in dynamic monitoring of rate.
#endif
#define MINRRATE (100*UPDATESEC)       //!< minimal radio rate
#define MEANRRATE (200*UPDATESEC)      //!< mean radio rate
#define MAXRRATE (600*UPDATESEC)       //!< maximal radio rate
#define MINSRATE (40*UPDATESEC)        //!< minimal scintillator rate
#define MEANSRATE (50*UPDATESEC)       //!< mean scintillator rate
#define MAXSRATE (80*UPDATESEC)        //!< max scintillator rate

//extern shm_gps;
//extern EV_DATA *eventbuf;      // buffer to hold the events
//extern GPS_DATA *gpsbuf;

int32_t tenrate[4]={0,0,0,0};  //!< rate of all channels, to be checked every "UPDATESEC" seconds
int32_t pheight[4]={0,0,0,0};  //!< summed pulseheight of all channels
int32_t n_events[4]={0,0,0,0}; //!< number of events contributing to summed pulse height

int16_t setsystime=0;          //!< check if system time is set


int32_t evgps=0;                //!< pointer to next GPS info
int32_t seczero=0;                //!< seczero keeps track of the number of seconds no data is read out

//int16_t cal_type=CAL_END;       //!< what to calibrate (END = nothing)
int32_t firmware_version;       //!< version of the firmware of the scope

int leap_sec = 0;


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
        //ir = scope_read_pps(ioff);
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



/*!
 \func int32_t scope_read_pps()
 \brief read pps, convert timestamp to GPS time, update circular GPS buffer
 \retval -7 error in reading the PPS
 \retval SCOPE_GPS OK
 */
/*
 int32_t scope_read_pps()  //27/7/2012 ok
 {
 int32_t rread,nread,ntry,i;
 struct tm tt;
 struct timeval tp;
 float *fp;
 unsigned short ppsrate;
 int32_t prevgps;
 
 nread = 2;                                    // again, already 2 bytes read!
 ntry = 0;
 gpsbuf[evgps].buf[0] = MSG_START;
 gpsbuf[evgps].buf[1] = ID_PARAM_PPS;
 gettimeofday(&tp,NULL);
 do{                                           // now read the remainder
 rread = scope_raw_read(&(gpsbuf[evgps].buf[nread]),PPS_LENGTH-nread);
 if(!rread) { usleep(10); ntry++; }
 else {ntry = 0;nread+=rread;}
 }while(nread <(PPS_LENGTH) &&ntry<MAXTRY);    // until the end or a timeout
 leap_sec = (int)(*(unsigned short *)&gpsbuf[evgps].buf[PPS_FLAGS]);
 printf("SCOPE_READ_PPS %d; leap = %d; Status 0x%x; rate = %d Pheight: ",tp.tv_sec,leap_sec,gpsbuf[evgps].buf[PPS_STATUS],*(short *)&(gpsbuf[evgps].buf[PPS_RATE]));
 for(i=0;i<4;i++) {
 if(n_events[i]>0) printf("%d ",pheight[i]/n_events[i]);
 pheight[i] = 0;
 n_events[i] = 0;
 }
 printf("\n");
 if((*(short *)&(gpsbuf[evgps].buf[PPS_RATE])) == 0) {
 seczero ++;
 } else{
 seczero = 0;
 }
 if(nread != PPS_LENGTH || gpsbuf[evgps].buf[nread-1] != MSG_END) {
 printf("Error in PPS %d %d %x\n",PPS_LENGTH,nread,gpsbuf[evgps].buf[nread-1]);
 for(i=0;i<nread;i++){
 if((i%8) == 0) printf("\n");
 printf("gpsbuf[%03d]=%02x\t",i,gpsbuf[evgps].buf[i] );
 }
 printf("\n");
 return(-7);                                 // GPS reading did not go smoothly
 }
 //scope_print_pps(gpsbuf[evgps].buf);
 //# if defined(CALFIRST)
 //if(*(short *)&(gpsbuf[evgps].buf[PPS_RATE]) == 0)   scope_print_pps(gpsbuf[evgps].buf);
 //#endif
 //ct 20140928 scope_fill_shadow(gpsbuf[evgps].buf);         // fill all the shadow config. lists
 tt.tm_sec = gpsbuf[evgps].buf[PPS_TIME+6];    // convert GPS into a number of seconds
 tt.tm_min = gpsbuf[evgps].buf[PPS_TIME+5];
 tt.tm_hour = gpsbuf[evgps].buf[PPS_TIME+4];
 tt.tm_mday = gpsbuf[evgps].buf[PPS_TIME+3];
 tt.tm_mon = gpsbuf[evgps].buf[PPS_TIME+2]-1;
 tt.tm_year = *(short *)(&gpsbuf[evgps].buf[PPS_TIME])-1900;
 gpsbuf[evgps].ts_seconds = (unsigned int)timegm(&tt);
 if(setsystime == 0){
 tp.tv_sec = gpsbuf[evgps].ts_seconds;
 settimeofday(&tp,NULL);
 setsystime = 1;
 }
 // Timestamp in Unix format
 // Convert UNIX time to GPS time in v3
 // NOTE: difftime() is apparently broken in this uclibc
 gpsbuf[evgps].ts_seconds -= (unsigned int)GPS_EPOCH_UNIX;
 //gpsbuf[evgps].ts_seconds -= leap_sec;
 //printf("PPS Time stamp = %d (%d)\n",gpsbuf[evgps].ts_seconds,GPS_EPOCH_UNIX);
 // time in GPS epoch CT 20110630 FIXED Number
 gpsbuf[evgps].CTP = (*(int *)&gpsbuf[evgps].buf[PPS_CTP])&0x7fffffff; //ok 25/7/2012
 gpsbuf[evgps].sync =(gpsbuf[evgps].buf[PPS_CTP]>>7)&0x1;
 // for 2.5 ns accuracy, get the clock-edge
 gpsbuf[evgps].quant = *(float *)(&gpsbuf[evgps].buf[PPS_QUANT]);
 prevgps = evgps-1;
 if(prevgps<0) prevgps = GPSSIZE-1;
 if((gpsbuf[evgps].ts_seconds -gpsbuf[prevgps].ts_seconds ) != 1){
 // can we calculate things accurately
 printf("ERROR I missed an C4 !!!!\n");
 }
 // length of clock-tick is (total time (ns))/(N clock ticks)
 gpsbuf[prevgps].clock_tick =  ((1000000000 -
 (gpsbuf[prevgps].quant-gpsbuf[evgps].quant)) / gpsbuf[prevgps].CTP);
 gpsbuf[prevgps].SCTP = gpsbuf[prevgps].CTP +
 (gpsbuf[prevgps].quant-gpsbuf[evgps].quant)/gpsbuf[prevgps].clock_tick;
 // corrected number of clock ticks/second
 *(shm_gps.next_read) = evgps;
 if ((tt.tm_sec%UPDATESEC) == 0) scope_check_rates(); // check rates every 10 seconds
 ppsrate = *(unsigned short *)&(gpsbuf[evgps].buf[PPS_RATE]);
 for(i=0;i<2;i++) {
 if(ppsrate< 400 || gpsbuf[evgps].rate[i]>(MEANRRATE/UPDATESEC))
 tenrate[i] += gpsbuf[evgps].rate[i];
 else
 tenrate[i] += (MEANRRATE/UPDATESEC); // do not know the real rate
 }
 for(i=2;i<4;i++) {
 if(ppsrate< 400 || gpsbuf[evgps].rate[i]>(MEANSRATE/UPDATESEC))
 tenrate[i] += gpsbuf[evgps].rate[i];
 else
 tenrate[i] += (MEANSRATE/UPDATESEC); // do not know what to do
 }
 evgps++;      // update the gpsbuf index, keeping in mind it is a circular buffer
 if(evgps>=GPSSIZE) evgps = 0;
 *(shm_gps.next_write) = evgps;
 for(i=0;i<4;i++)
 gpsbuf[evgps].rate[i] = 0;
 return(SCOPE_GPS);
 }
 */

void scope_main()
{
    scope_open();
    uint8_t buff[200];
    uint32_t length = sizeof(buff);
    
    
    unsigned short list_request=0x0C;
    
    scope_set_parameters(dig_mode_params,1);
    scope_set_parameters(readout_window_params,1);
    
    int i;
    for(i=0; i<nCh; i++){
        scope_set_parameters(ch_property_params[i],1);
        scope_set_parameters(ch_trigger_params[i],1);
    }
    
    
    for(i=0; i<5;i++){
        printf("________%d_______\n",i);
        
        scope_read(1);
        //ir =read(dev, (void *)buff, sizeof(buff));
        //read=scope_raw_read(&(buff),length);
        //printf("from buffer: %04x  %04x  %04x  %04x\n",buff[0],buff[1],buff[2],buff[3]);
        //usleep(100000);
        
    }
    
    
    /*
     for(i=0; i<PARAM_LIST_MAXSIZE; i++){
     printf("shadow: %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x\n",shadowlistR[1][i],shadowlistR[2][i],shadowlistR[3][i],shadowlistR[4][i],shadowlistR[8][i],shadowlistR[9][i],shadowlistR[10][i],shadowlistR[11][i],shadowlistR[12][i],shadowlistR[13][i]); //!< all parameters read from FPGA
     }
     */
    //scope_write((uint8_t *)ctrllist,sizeof(ctrllist));
    //usleep(1000);
    // scope_close();
    
    
}


