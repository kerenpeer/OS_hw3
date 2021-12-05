#include <linux/ioctl.h>

#ifndef MSGSLOT_H
#define MSGSLOT_H

// The major device number.
#define MAJOR_NUM 240
// Set the message of the device driver
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

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
  struct channel *channels;
} Msg_slot;

static Msg_slot* driver[256];

static int device_open(struct inode* inode, struct file* file);
static int device_release(struct inode* inode, struct file*  file);
static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset);
int find_channel(Msg_slot *ms, int id, Channel *c); 
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset);
static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param);
void buildC(Channel *c, unsigned long ioctl_param);
static int __init driver_init(void);
static void __exit driver_cleanup(void);

#endif