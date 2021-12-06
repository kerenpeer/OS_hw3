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
int find_channel(Msg_slot* ms, int id, Channel* c){
  Channel* head;
  printk("We just entered find_channel. S'emek\n");
  printk("id in find_channel: %d\n",id);
  head = ms -> channels; 
  while(head != NULL){
    printk("head id is: %d", head->id);
    printk("1"); 
    printk("2");  
    if(head -> id == id){
      c = head;
      return 0;
    }
    printk("3");  
    head = head -> next;
  }
  printk("4");  
  return -1;
}

//---------------------------------------------------------------
void buildC(Channel *c, unsigned long ioctl_param){
    printk("5");  
    c -> id = ioctl_param;
    printk("6");  
    c -> msg = NULL;
    printk("7");  
    c -> msg_len = -1;
    printk("8");  
    c -> next = NULL;
    printk("9");  
}

//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode* inode, struct file* file){
  int minor;
  Msg_slot* ms;

  printk("10");  
  ms = (Msg_slot*)kmalloc(sizeof(Msg_slot),GFP_KERNEL);
  printk("11");  
  memset(ms, 0, sizeof(Msg_slot));
  printk("12");  
  if (ms == NULL){
    printk("13");  
    return -EINVAL;
  }
  printk("14");  
  minor = iminor(inode);
  printk("15");  
  ms->minor = minor;
  printk("16");  
  ms->channels = NULL;
  printk("17");  
  driver[minor] = ms;
  printk("18");  
  printk("driver in %d is %d", minor, driver[minor]->minor); 
  printk("19");   
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
  int channel_id,minor, res, i, len;
  Msg_slot *ms;
  char *msg;
  Channel *c;
  printk("20");  
  len = -1;
  channel_id = *(int*)(file -> private_data);
  printk("21");  
  minor = iminor(file->f_path.dentry->d_inode);
  printk("22");  
  ms = driver[minor];
  printk("23");  
  if(!ms){
    printk("24");  
    return -EINVAL;     //validate error
  }
  c = (Channel*)kmalloc(sizeof(Channel),GFP_KERNEL);
  printk("25");  
  memset(c, 0, sizeof(Channel));
  printk("26");  
  if (c == NULL){
    printk("27");  
    return -EINVAL;
  }
  res = find_channel(ms, 4, c);
  printk("28");  
  if(res == -1){
    printk("29");  
    return -EINVAL;
  }
  msg = c -> msg;
  printk("30");  
  if(msg == NULL){
    printk("31");  
    return -EWOULDBLOCK;
  }
  printk("32");  
  len = c -> msg_len;
  printk("33");  
  if (len == -1 || len > length){
    printk("34");  
    return -ENOSPC;
  }
  for(i = 0; i < len ; i++){
    printk("35");  
    if(put_user(msg[i], &buffer[i]) != 0){
      return -EINVAL;     //validate error
    }
  }
  printk("36");  
  return len;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset){
  int channel_id, minor, res, i, len;
  Msg_slot *ms;
  //char *msg;
  Channel *c;
  
  printk("37");  
  channel_id = *(int*)(file -> private_data);
  printk("channel id is: %d",channel_id);  
  printk("38");  
  minor = iminor(file->f_path.dentry->d_inode);
  printk("minor is: %d\n",minor);  
  printk("39");  
  ms = driver[minor];
  if(!ms){
    printk("40");  
    return -EINVAL;     //validate error
  }
  printk("40.2");  
  c = (Channel*)kmalloc(sizeof(Channel),GFP_KERNEL);
  memset(c, 0, sizeof(Channel));
  printk("40.5");  
  if (c == NULL){
      printk("41");  
      return -EINVAL;
  }
  printk("41.1");  
  res = find_channel(ms, 4, c); /////fix!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  if(res == -1){
    printk("42");  
    return -EINVAL;
  }
  printk("42.3");  
  if(length <= 0 || length > 128){
    printk("43");  
    return -EMSGSIZE;
  }
  len = 0;
  c -> msg = (char*)kmalloc(length*sizeof(char),GFP_KERNEL);
  if(c -> msg == NULL){
    printk("43.1");  
    return -EINVAL;
  }
  printk("43.4");  
  for(i = 0; i < length ; i++){
    printk("44");  
      if(get_user((c -> msg)[i], &buffer[i]) != 0){
        printk("45");  
        return -EINVAL;     //validate error
      }
      len++;
  }
  printk("45.5");  
  c -> msg_len = len;
  printk("46");  
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
    printk("di1\n");
    if(driver[minor] == NULL){
      printk("di2\n");
      return -EINVAL;
    }
   // msg_s = driver[minor];
    printk("di3\n");
    c = (Channel*)kmalloc(sizeof(Channel),GFP_KERNEL);
    printk("di4\n");
    memset(c, 0, sizeof(Channel));
    printk("di5\n");
    if (c == NULL){
      printk("di6\n");
      return -EINVAL;
    }
    if(find_channel(driver[minor],ioctl_param,c) == -1){
      buildC(c, ioctl_param);
      printk("di7\n");
      file -> private_data = &ioctl_param;
      printk("new c id is %d",*(int*)(file -> private_data));
      printk("di8\n");
      if(driver[minor] ->channels == NULL){
        printk("di9\n");
        driver[minor] ->channels = c;
        printk("c id is: %d",driver[minor] ->channels->id);
      }
      else{
        printk("di10\n");
        prev =  driver[minor] -> channels;
        printk("di11\n");
        while(prev -> next != NULL){
          printk("di12\n");
          prev = prev -> next;
        }
        prev -> next = c;
        printk("di13\n");
      }
    }
  }
  else {
    printk("di14\n");
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
       kfree(&driver[i]);
    }
  }
  kfree(driver);
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(driver_init);
module_exit(driver_cleanup);
