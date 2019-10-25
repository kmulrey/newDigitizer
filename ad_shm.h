/***
 DAQ shared memory definitions
 Version:0.1
 Date: 26/11/2012
 Author: Charles Timmermans, Nikhef/Radboud University
 
 Altering the code without explicit consent of the author is forbidden
 ***/
#include <stdint.h>
#include <unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>

typedef struct{
    int shmid;
    int *next_read;
    int *next_readb; // t3 messages are read by 2 processes
    int *next_write;
    int *nbuf;
    int *size;
    char *buf;
    uint16_t *Ubuf;
}shm_struct;

int ad_shm_create(shm_struct *ptr,int nbuf,int size);
void ad_shm_delete(shm_struct *ptr);

