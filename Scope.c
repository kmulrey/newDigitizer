#include <stdio.h>
#include <stdlib.h>
//#include "add.h"
//#include <iostream>
#include <sys/wait.h>
//#include <sys/time.h>
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
#include "time.h"
#include "ad_shm.h"
#include "Scope.h"
#include "Socket.h"



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



extern shm_struct shm_ev,shm_gps;
//extern EV_DATA *eventbuf1;      // buffer to hold the events
extern GPS_DATA *gpsbuf;
extern EV_DATA *gpsbuf2;

extern socket_connection sock_listen;
extern socket_connection sock_send;


//socket_connection sock_listen1;
//socket_connection sock_send2;



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
    //printf("rawbuf[1] = %x\n",rawbuf[1]);
    
    
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
        ir = scope_read_pps(ioff);
        //printf("raw buff: %x\n",rawbuf[2]);
        printf("----->PPS \n");
        if(evgps>0){
            //printf("gps number:    %lu\n",sizeof(gpsbuf[evgps-1].buf));
            Write_Data(sock_send.sockfd,gpsbuf[evgps-1].buf,sizeof(gpsbuf[evgps-1].buf));

        }
        
        
        
        
        
        
        //scope_calc_evnsec();
       // printf("          %x\n",gpsbuf[evgps].buf[2]);
        //gpsbuf[evgps]
        return(ir);
    }
    else if(rawbuf[1] == ID_PARAM_EVENT)
    {
        ir=scope_read_event(ioff);
        printf("----->EVENT!!!! \n");
        //Write_Data(sock_send.sockfd,rawbuf);
        if(*(shm_ev.next_write)>0){
           // printf("event number:    %d  %x  %x  %x\n",*(shm_ev.next_write),gpsbuf2[*(shm_ev.next_write)-1].buf[0],gpsbuf2[*(shm_ev.next_write)-1].buf[1],gpsbuf2[*(shm_ev.next_write)-1].buf[2]);
            Write_Data(sock_send.sockfd,gpsbuf2[*(shm_ev.next_write)-1].buf,sizeof(gpsbuf2[*(shm_ev.next_write)-1].buf));
            
        }
        
        //printf("raw buff: %x\n",rawbuf[2]);

        return(ir);
        
    }
    
    
    
    
    
    
    
    
    //else if(rawbuf[1] == ID_PARAM_ERROR) return(scope_read_error(ioff));
   // printf("ERROR Identifier = %x\n",rawbuf[1]);
  //  return(-3);
    else{// bad identifier read
        return 0;}
}


/*!
 \func int32_t scope_read_pps()
 \brief read pps, convert timestamp to GPS time, update circular GPS buffer
 \retval -7 error in reading the PPS
 \retval SCOPE_GPS OK
 */

 int32_t scope_read_pps()  //27/7/2012 ok
 {
     int32_t rread,nread,ntry,i;
     struct tm tt;
     struct timeval tp;
     float *fp;
     unsigned short ppsrate;
     int32_t prevgps;
     //printf("%x\n",gpsbuf[0]);

     nread = 2;                                    // again, already 2 bytes read!
     ntry = 0;
     //printf("------>in here\n");
     gpsbuf[evgps].buf[0] = MSG_START;
     gpsbuf[evgps].buf[1] = ID_PARAM_PPS;
     //printf("........%x\n",gpsbuf[evgps].buf[0]);

     gettimeofday(&tp,NULL);
     
     do{                                           // now read the remainder
         rread = scope_raw_read(&(gpsbuf[evgps].buf[nread]),PPS_LENGTH-nread);
         if(!rread) { usleep(10); ntry++; }
        else {ntry = 0;nread+=rread;}
     }while(nread <(PPS_LENGTH) &&ntry<MAXTRY);    // until the end or a timeout
     leap_sec = (int)(*(unsigned short *)&gpsbuf[evgps].buf[PPS_FLAGS]);
     //printf("SCOPE_READ_PPS %d\n",tp.tv_sec);
     
     /*
     for(i=0;i<4;i++) {
         if(n_events[i]>0) printf("%d ",pheight[i]/n_events[i]);
        pheight[i] = 0;
         n_events[i] = 0;
     }
      */
     printf("\n");
     if((*(short *)&(gpsbuf[evgps].buf[PPS_RATE])) == 0) {
         seczero ++;
     }
     else{
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
     printf("PPS Time stamp = %d (%d)\n",gpsbuf[evgps].ts_seconds,GPS_EPOCH_UNIX);
     // time in GPS epoch CT 20110630 FIXED Number
     gpsbuf[evgps].CTP = (*(int *)&gpsbuf[evgps].buf[PPS_CTP])&0x7fffffff; //ok 25/7/2012
     gpsbuf[evgps].sync =(gpsbuf[evgps].buf[PPS_CTP]>>7)&0x1;
        // for 2.5 ns accuracy, get the clock-edge
     gpsbuf[evgps].quant = *(float *)(&gpsbuf[evgps].buf[PPS_QUANT]);
     prevgps = evgps-1;
     if(prevgps<0) prevgps = GPSSIZE-1;
     if((gpsbuf[evgps].ts_seconds -gpsbuf[prevgps].ts_seconds ) != 1){
     // can we calculate things accurately
    //printf("%d\n",evgps);
    //printf("diff %d   %d\n",gpsbuf[evgps].ts_seconds,gpsbuf[prevgps].ts_seconds);

     printf("ERROR I missed an C4 !!!!\n");
    }
     
     // length of clock-tick is (total time (ns))/(N clock ticks)
     gpsbuf[prevgps].clock_tick =  ((1000000000 -
                                     (gpsbuf[prevgps].quant-gpsbuf[evgps].quant)) / gpsbuf[prevgps].CTP);
     gpsbuf[prevgps].SCTP = gpsbuf[prevgps].CTP +
     (gpsbuf[prevgps].quant-gpsbuf[evgps].quant)/gpsbuf[prevgps].clock_tick;
     // corrected number of clock ticks/second
     
     //printf("clock ticks: %d\n",gpsbuf[prevgps].CTP);
     
     *(shm_gps.next_read) = evgps;
     //if ((tt.tm_sec%UPDATESEC) == 0) scope_check_rates(); // check rates every 10 seconds
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

/*!
 \func void scope_fill_ph(uint8_t *buf)
 \brief for each channel add to the summed pulse height, also add to the number of events
 */
void scope_fill_ph(uint8_t *buf)
{
    int32_t i,istart,iend,iadc,len[4];
    int16_t sb;
    int16_t vmax=-8100;
    int16_t vmin = 8100;
    
    for(i=0;i<4;i++) {
        len[i] = *(short *)&buf[EVENT_LENCH1+2*i];
    }
    istart = EVENT_ADC;
    for(i=0;i<4;i++){
        vmax = -8100;
        vmin = 8100;
        iend = istart+2*len[i];
        for(iadc=istart;iadc<iend;iadc+=2) {
            sb = *(short *)&buf[iadc];
            if(sb>vmax) vmax = sb;
            if(sb<vmin) vmin = sb;
        }
        // printf("Channel %d, Start %d End %d (%d %d)\n",i+1,istart,iend,vmax,vmin);
        if(vmax>vmin){
            pheight[i]+=(vmax-vmin);
            n_events[i] +=1;
        }
        istart = iend;
    }
}


int scope_read_event(int32_t ioff)
{
    int32_t rread,nread,ntry;
    struct tm tt;
    int32_t prevgps;
    uint16_t length;
    int next_write = *(shm_ev.next_write);
    //printf("next write: %d\n",next_write);
    //printf("%d\n",gpsbuf2[0]);
    
    gpsbuf2[next_write].buf[0] = MSG_START;
    gpsbuf2[next_write].buf[1] = ID_PARAM_EVENT;

   // eventbuf[next_write].buf[0] = MSG_START;
    
    
    //eventbuf[next_write].buf[1] = ID_PARAM_EVENT;
    
     
    scope_raw_read(&(gpsbuf2[next_write].buf[2]),2);
    
    nread = 4; // length andA4 words
    length = *(unsigned short *)&(gpsbuf2[next_write].buf[2]);
    
    if(length>MAX_READOUT) return(-10); // too long
    ntry = 0;
    
    do{            //while absolutely needed as blocks are read out
        rread = scope_raw_read(&(gpsbuf2[next_write].buf[nread]),length-nread);
        if(!rread) { usleep(10); ntry++; }
        else {ntry = 0; nread+=rread;}
    }while(nread <length &&ntry<MAXTRY);              // until the end or timeout
    if(nread < length) {
        printf("nread = %d length = %d rread = %d errno = %d\n",nread,length,rread,errno);
        return(-11); // an error if not all is read
    }
    
    gpsbuf2[next_write].evsize = (gpsbuf2[next_write].buf[EVENT_BCNT]+(gpsbuf2[next_write].buf[EVENT_BCNT+1]<<8)); //the total length in bytes from scope
    //scope_print_event(gpsbuf2[next_write].buf);  // can be commented
    
    
    //scope_send_event_katie(eventbuf[next_write].buf); //katie
    
    
    
    scope_fill_ph(gpsbuf2[next_write].buf);
    tt.tm_sec = gpsbuf2[next_write].buf[EVENT_GPS+6];    // Convert GPS in a number of seconds
    tt.tm_min = gpsbuf2[next_write].buf[EVENT_GPS+5];
    tt.tm_hour = gpsbuf2[next_write].buf[EVENT_GPS+4];
    tt.tm_mday = gpsbuf2[next_write].buf[EVENT_GPS+3];
    tt.tm_mon = gpsbuf2[next_write].buf[EVENT_GPS+2]-1;
    tt.tm_year = *(short *)&(gpsbuf2[next_write].buf[EVENT_GPS]) - 1900;
    gpsbuf2[next_write].ts_seconds = (unsigned int)timegm(&tt);
    // Timestamp in Unix format
    // Convert UNIX time to GPS time in v3
    // NOTE: difftime() is apparently broken in this uclibc
    gpsbuf2[next_write].ts_seconds -= (unsigned int)GPS_EPOCH_UNIX;
    //eventbuf[next_write].ts_seconds -= leap_sec;
    // time in GPS epoch CT 20110630 fixed number!
    gpsbuf2[next_write].CTD = *(int *)&(gpsbuf2[next_write].buf[EVENT_CTD]);
    // fill the clock tick of the event
    prevgps = evgps-1;    // from the previous GPS get a first guess of the event time
    if(prevgps<0) prevgps = GPSSIZE-1;
    prevgps = prevgps-1;  // from the previous GPS get a first guess of the event time
    if(prevgps<0) prevgps = GPSSIZE-1;    //SHOULD IT BE TWICE (I think so)
    gpsbuf2[next_write].t2_nanoseconds = gpsbuf[prevgps].clock_tick*gpsbuf2[next_write].CTD;
    gpsbuf2[next_write].t3_nanoseconds = 0;  // the real time is not (yet) known
    gpsbuf2[next_write].t3calc = 0;          // and has not yet been calculated
    // and the trigger setting in gpsdata
    if((gpsbuf2[next_write].buf[EVENT_TRIGMASK+1]&0x1)!=0) gpsbuf[evgps].rate[0] ++;
    if((gpsbuf2[next_write].buf[EVENT_TRIGMASK+1]&0x2)!=0) gpsbuf[evgps].rate[1] ++;
    if((gpsbuf2[next_write].buf[EVENT_TRIGMASK+1]&0x4)!=0) gpsbuf[evgps].rate[2] ++;
    if((gpsbuf2[next_write].buf[EVENT_TRIGMASK+1]&0x8)!=0) gpsbuf[evgps].rate[3] ++;
    //
    
    if(gpsbuf[prevgps].clock_tick != 0 &&
       gpsbuf2[next_write].t2_nanoseconds != -1) {
        next_write +=ioff;   // update the buffer counter if needed
    }
    
    if(next_write>=BUFSIZE) next_write = 0;       // remember: eventbuf is a circular buffer
    *(shm_ev.next_write) = next_write;
    
    
    return(SCOPE_EVENT);                  // success!
}




void read_fake_file(char *name){
    printf("opening file:   %s\n",name);
    
    
    FILE *fptr;
    fptr=fopen(name,"r");
    
    if(fptr == NULL) {
        printf("Unable to open file!");
    }
    
    
    char buffer[255];
    
    int len=0;
    uint8_t one;
    uint8_t two;
    uint8_t three;
    int end=0;
    
    int k=0;
    while(fgets(buffer, 255, (FILE*) fptr)&& k<MAX_READOUT) {
        fake_event[k]=0;
        len = strlen(buffer);
        if( buffer[len-1] == '\n' ){
            buffer[len-1] = 0;}
        
        uint8_t x;
        
        sscanf(buffer, "%x", &x);
        fake_event[k]=x;
        if(k==0){one=x;}
        if(k==1){two=x;}
        if(k==2){three=x;}
        
        if(x==102){
            //printf("-->%x\n", fake_event[k]);
            end=k;
        }
        k++;
    }
    
    fake_event[0]=one;
    fake_event[1]=two;
    fake_event[2]=three;
    
    fclose(fptr);
    
    
    
    
}

















void scope_main()
{
    scope_open();
    uint8_t buff[200];
    uint32_t length = sizeof(buff);
    
  
   
    
    
    unsigned short list_request=0x0C;
    int i;
    
    
    scope_set_parameters(dig_mode_params,1);
    scope_set_parameters(readout_window_params,1);
    
    
    for(i=0; i<nCh; i++){
        scope_set_parameters(ch_property_params[i],1);
        scope_set_parameters(ch_trigger_params[i],1);
    }
    

 /*
    
    for(i=0; i<5;i++){
        printf("________%d_______\n",i);
        
    }
    */
    /*
    printf("event number: %d \n",gpsbuf2[0].event_nr);
    printf("event number: %d \n",gpsbuf2[1].event_nr);
    
    printf("event size: %d \n",gpsbuf2[0].evsize);
    printf("event size: %d \n",gpsbuf2[1].evsize);

    
    printf("event CTD: %d \n",gpsbuf2[0].CTD);
    printf("event CTD: %d \n",gpsbuf2[1].CTD);
    
    printf("event buff 1,2,3: %d  %d  %d \n",gpsbuf2[0].buf[0],gpsbuf2[0].buf[1],gpsbuf2[0].buf[10]);
    printf("event buff 1,2,3: %d  %d  %d  \n",gpsbuf2[1].buf[0],gpsbuf2[1].buf[1],gpsbuf2[1].buf[10]);
    */
    /*
    FILE *fptr;
    fptr=fopen("test3.txt","w");
    
    for(i=0; i<MAX_READOUT; i++){
        fprintf(fptr,"%x\n",gpsbuf2[8].buf[i]);
    }
    
    
    
    fclose(fptr);
     */
    
    /*
     for(i=0; i<PARAM_LIST_MAXSIZE; i++){
     printf("shadow: %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x\n",shadowlistR[1][i],shadowlistR[2][i],shadowlistR[3][i],shadowlistR[4][i],shadowlistR[8][i],shadowlistR[9][i],shadowlistR[10][i],shadowlistR[11][i],shadowlistR[12][i],shadowlistR[13][i]); //!< all parameters read from FPGA
     }
     */
    //scope_write((uint8_t *)ctrllist,sizeof(ctrllist));
    //usleep(1000);
    // scope_close();
    
    
    sleep(2);
    
    /////////////// loop with main PC
    
  
    //char filename[]="test_data/test1.txt";
    //read_fake_file(filename);
    
    int c1=0;
    int r;
    unsigned char buff0[100];
    bzero(buff, sizeof(buff));
    buff0[0]=0;//x99;
    struct timeval stop, start;
    float dur=0;

    gettimeofday(&start, NULL);
    
    while(1){

        scope_read(1);

        
        // sleep(1);

        //Write_Data(sock_send.sockfd,buff0);
        r=func_read_message(sock_listen.sockfd);
        if(r==1){
            printf("got message to break\n");
            break;
            
        }
        
        
        /////////////////////////// manage loop ///////////////////////////
        c1++;
        gettimeofday(&stop, NULL);
        dur= (double) (stop.tv_sec - start.tv_sec) * 1000 + (double) (stop.tv_usec - start.tv_usec) / 1000;
        
        if(dur>1500.0){
            Write_Data(sock_send.sockfd,buff0,1);
            gettimeofday(&start, NULL);
            //printf(".............\n");

        }
      
    
        if(c1>5000){
            break;
        }
        ///////////////////////////
      
    }

    printf("leaving scope\n");

    
}




