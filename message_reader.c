
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "message_slot.h"

int main (int argc, char* argv[] ){
    if (argc<3){
        fprintf(stderr,"not enough argument, submit 3 arguments next time");
        exit(FAILED);
    }
    int fd,ret_val;
    unsigned long channel_id;
    char buffer[BUF_LEN];

    fd = open(argv[1], O_RDONLY);
    if (fd<0){
        perror("error opening the file");
        exit(FAILED);
    }
    channel_id = atoi(argv[2]);
    ret_val = ioctl(fd,MSG_SLOT_CHANNEL, channel_id);
    if (ret_val != SUCCESS){
        perror("error setting the channel, wrong channel id");
        exit(FAILED);
    }

    ret_val = read(fd,buffer, BUF_LEN);
    if(ret_val<0){
        perror("error reading from device");
        exit(FAILED);
    } else {
        if (write(STDOUT_FILENO, buffer, ret_val) == -1){
            perror("error writing to the console");
        }
    }

    close(fd);
    return SUCCESS;
}