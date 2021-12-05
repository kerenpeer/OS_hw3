#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "message_slot.h"

/**
 * @brief Will read a message from a channel
 * 
 * @param argc - number of arguments in argsv. When correct - should be 3. 
 * @param argv - parameters. argv[1] = message slot file path. argv[2] =  the target message channel id. 
 * @return int 
 */
int main(int argc, char *argv[]){
    char *file_path, message[BUF_LEN];
    int channel_id, fd, ioctl_outcome;

    if(argc != 3){
        perror("Wrong amount of arguments.");
        exit(1);
    }
    file_path = argv[1];
    channel_id = atoi(argv[2]);

    fd = open(file_path, O_RDONLY);
    if(fd < 0){
        perror("Error in opening file descriptor for read");
        exit(1);
    }
    ioctl_outcome = ioctl(fd,MSG_SLOT_CHANNEL,channel_id);
    if(ioctl_outcome == -1){  
        perror("Error ioctrl cmd for read");
        exit(1);
    }
    ioctl_outcome = read(fd,message,BUF_LEN);
    if(ioctl_outcome == -1){
        perror("Error in read");
        exit(1);
    }
    if (write(1, message,ioctl_outcome) != ioctl_outcome) {
		perror("Message was too big for buffer");
		exit(1);
	}
    close(fd);
    exit(0);
}