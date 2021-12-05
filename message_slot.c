#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>
#include <linux/errno.h>
// Our custom definitions of IOCTL operations
#include "message_slot.h"
// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

MODULE_LICENSE("GPL");

//================== HELP FUNCTIONS ===========================
int find_channel(Msg_slot* ms, int id, Channel* c){
  Channel* head;

  head = ms -> channels;
  while(head != NULL){
    if(head -> id == id){
      c = head;
      return 0;
    }
    head = head -> next;
  }
  return -1;
}

//---------------------------------------------------------------
void buildC(Channel *c, unsigned long ioctl_param){
    c -> id = ioctl_param;
    c -> msg = NULL;
    c -> msg_len = -1;
    c -> next = NULL;
}

//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode* inode, struct file* file){
  int minor;
  Msg_slot* ms;

  ms = (Msg_slot*)kmalloc(sizeof(Msg_slot),GFP_KERNEL);
  memset(ms, 0, sizeof(Msg_slot));
  if (ms == NULL){
      return -EINVAL;
  }
  minor = iminor(inode);
  ms -> minor = minor;
  ms -> channels = NULL;
  
  driver[minor] = ms;
  
  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release(struct inode* inode, struct file*  file){
  return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset){
  int *channel_id,minor, res, i, len;
  Msg_slot *ms;
  char *msg;
  Channel *c;

  len = -1;
  channel_id = (int*)file -> private_data;
  minor = iminor(file->f_path.dentry->d_inode);
  ms = driver[minor];
  if(ms == NULL){
    return -EINVAL;     //validate error
  }
  c = (Channel*)kmalloc(sizeof(Channel),GFP_KERNEL);
  memset(c, 0, sizeof(Channel));
  if (c == NULL){
      return -EINVAL;
  }
  res = find_channel(ms, *channel_id, c);
  if(res == -1){
    return -EINVAL;
  }
  msg = c -> msg;
  if(msg == NULL){
      return -EWOULDBLOCK;
  len = c -> msg_len;
  }
  if (len == -1 || len > length){
     return -ENOSPC;
  }
  for(i = 0; i < len ; i++){
      if(put_user(msg[i], &buffer[i]) != 0){
        return -EINVAL;     //validate error
      }
  }
  return len;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset){
  int *channel_id, minor, res, i, len;
  Msg_slot *ms;
  char *msg;
  Channel *c;

  channel_id = (int*)file -> private_data;
  minor = iminor(file->f_path.dentry->d_inode);
  ms = driver[minor];
  if(ms == NULL){
    return -EINVAL;     //validate error
  }
  c = (Channel*)kmalloc(sizeof(Channel),GFP_KERNEL);
  memset(c, 0, sizeof(Channel));
  if (c == NULL){
      return -EINVAL;
  }
  res = find_channel(ms, *channel_id, c);
  if(res == -1){
    return -EINVAL;
  }
  if(length <= 0 || length > 128){
    return -EMSGSIZE;
  }
  len = 0;
  msg = c -> msg;
  for(i = 0; i < length ; i++){
      if(get_user(msg[i], &buffer[i]) != 0){
        return -EINVAL;     //validate error
      }
      len++;
  }
  c -> msg_len = len;
  return len;
}

//----------------------------------------------------------------
static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param){
  int minor;
  Msg_slot *msg_s;
  Channel *c, *prev;

  // Switch according to the ioctl called
  if(MSG_SLOT_CHANNEL == ioctl_command_id && ioctl_param != 0){
    // Get the parameter given to ioctl by the process
    minor = iminor(file->f_path.dentry->d_inode);
    if(driver[minor] == NULL){
       return -EINVAL;
    }
    msg_s = driver[minor];
    c = (Channel*)kmalloc(sizeof(Channel),GFP_KERNEL);
    memset(c, 0, sizeof(Channel));
    if (c == NULL){
      return -EINVAL;
    }
    buildC(c, ioctl_param);
    file -> private_data = (void*) ioctl_param;
    if(msg_s -> channels == NULL){
      msg_s -> channels = c;
    }
    else{
      prev =  msg_s -> channels;
      while(prev -> next != NULL){
        prev = prev -> next;
      }
      prev -> next = c;
    }
  }
  else {
    return -EINVAL;
  }
  return SUCCESS;
}

//==================== DEVICE SETUP =============================
// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops ={
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,
};

// Initialize the module - Register the character device
static int __init driver_init(void){
  int rc = -1;
 
  // Register driver capabilities. Obtain major num
  rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

  // Negative values signify an error
  if( rc < 0 ){
    printk( KERN_ERR "%s registraion failed for  %d\n",
                       DEVICE_FILE_NAME, MAJOR_NUM );
    return rc;  //needed? 
  }
  memset(driver,0,256*sizeof(Msg_slot*));
  return 0;
}

//---------------------------------------------------------------
// Terminate module and free all alocated memory
static void __exit driver_cleanup(void){
  // Unregister the device
  // Should always succeed
  int i;
  Channel *Clist, *tmp;
  for(i=0; i < 256; i++){
    if(driver[i] != NULL){
       Clist = driver[i]-> channels;
       while(Clist != NULL){
         kfree(Clist -> msg);
         tmp = Clist -> next;
         kfree(Clist);
         Clist = tmp;
       }
       kfree(driver[i]);
    }
  }
  kfree(driver);
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(driver_init);
module_exit(driver_cleanup);
