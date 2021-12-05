#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "message_slot.h"

/**
 * @brief Will send a message through a channel
 * 
 * @param argc - length of argv. When correct should be 4.
 * @param argv - parameters. argv[1] = message slot file path. argv[2] =  the target message channel id. argv[3] =  the message to pass.
 * @return int 
 */
int main(int argc, char *argv[]){
    char *file_path, *message;
    int channel_id, fd, ioctl_outcome, messlen;

    if(argc != 4){
        perror("Wrong amount of arguments.");
        exit(1);
    }
    file_path = argv[1];
    message = argv[3];  // can I do this if not necessarily a string? 
    messlen = strlen(message);
    channel_id = atoi(argv[2]);

    fd = open(file_path, O_WRONLY);
    if(fd < 0){
        perror("Error in opening file descriptor for write");
        exit(1);
    }
    ioctl_outcome = unlocked_ioctl(fd,MSG_SLOT_CHANNEL,channel_id);
    if(ioctl_outcome == -1){  
        perror("Error ioctrl cmd for write");
        exit(1);
    }
    ioctl_outcome = write(fd,message, messlen);
    if(ioctl_outcome != messlen){
        perror("Error in write");
        exit(1);
    }
    close(fd);
    exit(0);

}