#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>

// The major device number.
#define MAJOR_NUM 240
// Set the message of the device driver
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "Keren's_dev"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "message_slot_dev"
#define SUCCESS 0

#endif