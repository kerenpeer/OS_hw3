/**
 * @file message_slot.c
 * @author Keren Pe'er
 * @brief 
 * @version 0.1
 * @date 2021-12-08
 * 
 * @copyright Copyright (c) 2021
 * 
 */
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
static Msg_slot* driver[256];

//================== HELP FUNCTIONS ===========================
Channel* find_channel(Msg_slot* ms, int id){
  Channel* head;
 
  head = ms -> channels; 
  while(head != NULL){
    if(head -> id == id){
      return head;
    }
    head = head -> next;
  }
  return NULL;
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

  minor = iminor(inode);
  if(driver[minor] == NULL){
    ms = (Msg_slot*)kmalloc(sizeof(Msg_slot),GFP_KERNEL);
    memset(ms, 0, sizeof(Msg_slot));
    if (ms == NULL){
      return -EINVAL;
    } 
    ms->minor = minor;
    ms->channels = NULL;
    driver[minor] = ms;
  } 
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
  int channel_id,minor, i, len;
  Msg_slot *ms;
  Channel *res;
  
  len = -1;
  channel_id = (unsigned long)file -> private_data;
  minor = iminor(file->f_path.dentry->d_inode); 
  if(driver[minor] == NULL){
    return -EINVAL;     //validate error
  }
  ms = driver[minor];
  res = find_channel(ms, channel_id);
  if(res == NULL){
    return -EINVAL;
  }
  len = res -> msg_len;
  if(res -> msg == NULL){
    return -EWOULDBLOCK;
  }  
  if (len == -1 || len > length){
    return -ENOSPC;
  }
  for(i = 0; i < len ; i++){
    if(put_user((res -> msg)[i], &buffer[i]) != 0){
      return -EINVAL;     //validate error
    }
  }
  return len;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset){
  int channel_id, minor, i, len;
  Msg_slot *ms;
  char *prevMsg;
  Channel *res;
  
  channel_id = (unsigned long) file -> private_data;
  minor = iminor(file->f_path.dentry->d_inode);
  if(driver[minor] == NULL){
    return -EINVAL;     //validate error
  }
  ms = driver[minor]; 
  res = find_channel(ms, channel_id);
  if(res == NULL){
    return -EINVAL;
  }
  if(length <= 0 || length > 128){
    return -EMSGSIZE;
  }
  len = 0;
  prevMsg = res -> msg;
  kfree(prevMsg);
  res-> msg = (char*)kmalloc(length*sizeof(char),GFP_KERNEL);
  if(res -> msg == NULL){
    return -EINVAL;
  }
  for(i = 0; i < length ; i++){
      if(get_user((res -> msg)[i], &buffer[i]) != 0){
        return -EINVAL;     //validate error
      }
      len++;
  }
  res -> msg_len = len;
  return len;
}

//----------------------------------------------------------------
static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param){
  int minor;
 // Msg_slot *msg_s;
  Channel *c, *prev;

  // Switch according to the ioctl called
  if(MSG_SLOT_CHANNEL == ioctl_command_id && ioctl_param != 0){
    // Get the parameter given to ioctl by the process
    minor = iminor(file->f_path.dentry->d_inode);
    if(driver[minor] == NULL){
      return -EINVAL;    }
    file -> private_data = (void*) ioctl_param;
    if(find_channel(driver[minor],ioctl_param) == NULL){
      c = (Channel*)kmalloc(sizeof(Channel),GFP_KERNEL);
      memset(c, 0, sizeof(Channel));
      if (c == NULL){
        return -EINVAL;
      }
      buildC(c, ioctl_param);
      if(driver[minor] ->channels == NULL){
        driver[minor] ->channels = c;
      }
      else{
        prev =  driver[minor] -> channels;
        while(prev -> next != NULL){
          prev = prev -> next;
        }
        prev -> next = c;
      }
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
  int i, rc = -1;
 
  // Register driver capabilities. Obtain major num
  rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

  // Negative values signify an error
  if( rc < 0 ){
    printk( KERN_ERR "%s registraion failed for  %d\n",
                       DEVICE_FILE_NAME, MAJOR_NUM );
    return rc;  //needed? 
  }
 //driver = (Msg_slot*)kmalloc(256 * sizeof(Msg_slot),GFP_KERNEL);
// memset(driver,0,256*sizeof(Msg_slot));
 // if(!driver){
   // printk("Problem in driver");
  //}
  //if(driver != NULL){
    //printk("opened driver");
  //}
  for (i=0; i < 256; i++){
    driver[i] = (Msg_slot*)kmalloc(sizeof(Msg_slot), GFP_KERNEL);
    driver[i] = NULL;
  }
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
      Clist = driver[i] -> channels;
      while(Clist != NULL){
        kfree(Clist -> msg);
        tmp = Clist -> next;
        kfree(Clist);
        Clist = tmp;
      }
      kfree(driver[i]);
    }
  }
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(driver_init);
module_exit(driver_cleanup);
