/***
 DAQ Main project definitions
 Version:0.2
 Date: 18/2/2013
 Author: Charles Timmermans, Nikhef/Radboud University
 
 Altering the code without explicit consent of the author is forbidden
 ***/
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ad_shm.h"

#define MAXLS 60 //max number of Local Stations
#define ERROR -1
#define NORMAL 1
#define DEFAULT_CONFIGFILE "conf/Adaq.conf"

typedef struct{
    int LSid; // station number
    char LSip[20]; // IP address
    int LSport;   // port to connect to
    int LSsock;
    time_t LSTconnect;
    struct sockaddr_in  LSaddress;
    socklen_t LSalength;
}LSInfo;

#define NT2BUF 500 //one per LS
#define T2SIZE 1000 //Max. size (in shorts) for T2 info in 1 message

#define NT3BUF 50 // max 50 T3's in 20 sec
#define T3SIZE (6+3*MAXLS) //Max. size (in shorts) for T3 info in 1 message

#define NEVBUF 10 // maximal 10 event buffers
#define EVSIZE 40000 //Max. size (in shorts) for evsize for each LS

#define CMDBUF 200 // leave 20 command buffers
#define CMDSIZE 5000 //Max. size (in shorts) for command (should be able to hold config file)

#ifdef _MAINDAQ
LSInfo lsinfo[MAXLS];
int tot_ls;
shm_struct shm_t2;
shm_struct shm_t3;
shm_struct shm_cmd;
shm_struct shm_eb;
//next EB parameters
int eb_run = 1;
int eb_run_mode = 0;
int eb_max_evts = 10;
char eb_dir[80];
//T3 parameters
int t3_rand = 0;
#else
extern LSInfo lsinfo[MAXLS];
extern int tot_ls;
extern shm_struct shm_t2;
extern shm_struct shm_t3;
extern shm_struct shm_eb;
extern shm_struct shm_cmd;
extern int eb_run ;
extern int eb_run_mode;
extern int eb_max_evts ;
extern char eb_dir[80];
extern int t3_rand;
#endif

