
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "message_slot.h"

int main (int argc, char* argv[] ){
    if (argc<4){
        fprintf(stderr,"not enough argument, submit 4 arguments next time");
        exit(FAILED);
    }
    int fd,ret_val;
    unsigned int channel_id;
    long len = strlen(argv[3]);

    fd = open(argv[1], O_WRONLY);
    if (fd<0){
        perror("error opening the file");
        exit(FAILED);
    }
    channel_id = atoi(argv[2]);
    ret_val = ioctl(fd,MSG_SLOT_CHANNEL, channel_id);
    if (ret_val != SUCCESS){
        perror("error updating IOCTL");
        exit(FAILED);
    }

    ret_val = write(fd,argv[3], len);
    if(ret_val!=len){
        perror("error writing the message");
        exit(FAILED);
    }

    close(fd);
    return SUCCESS;
}