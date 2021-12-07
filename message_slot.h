#include <linux/ioctl.h>

#ifndef MSGSLOT_H
#define MSGSLOT_H

// The major device number.
#define MAJOR_NUM 240
// Set the message of the device driver
//#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#define MSG_SLOT_CHANNEL 1
#define DEVICE_RANGE_NAME "Keren's_dev"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "message_slot_dev"
#define SUCCESS 0

typedef struct channel{
  int id;
  char *msg;
  int msg_len;
  struct channel* next;
} Channel;

typedef struct msg_slot{
  int minor;
  Channel *channels;
} Msg_slot;

#endif