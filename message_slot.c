// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>     /*for kmalloc, kfree*/
#include "message_slot.h"

MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations

//a struct for a linked list node that will be use to save the channels
typedef struct channel_node{
    int channel_id;
    char buffer[128];
    int text_length;
    struct channel_node *next_channel;
}channel_node;

// a struct containing an array of pointers to all the 256 message slot devices
typedef struct message_slot{
   channel_node *first_channel;
   channel_node *curr_channel;
}message_slot;



static message_slot *slots[256] = {NULL};



//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
    int minor = (int)iminor(inode);
    if (slots[minor] == NULL){ //TODO: maybe it has to be minor-1
        message_slot *new_slot = (message_slot *)kmalloc(sizeof(message_slot), GFP_KERNEL);
        if (new_slot == NULL){
            printk("kmalloc error\n");
            return -EINVAL;
        }
        new_slot->curr_channel = NULL;
        new_slot->first_channel= NULL;
        slots[minor] = new_slot;
    }
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
    unsigned long flags; // for spinlock
    printk("Invoking device_release(%p,%p)\n", inode, file);

    // ready for our next caller
    spin_lock_irqsave(&device_info.lock, flags);
    --dev_open_flag;
    spin_unlock_irqrestore(&device_info.lock, flags);
    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
size_t       length,
        loff_t*      offset )
{
// read doesnt really do anything (for now)
printk( "Invocing device_read(%p,%ld) - "
"operation not supported yet\n"
"(last written - %s)\n",
file, length, the_message );
//invalid argument error
return -EINVAL;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
size_t             length,
        loff_t*            offset)
{
int i;
printk("Invoking device_write(%p,%ld)\n", file, length);
for( i = 0; i < length && i < BUF_LEN; ++i )
{
get_user(the_message[i], &buffer[i]);
if( 1 == encryption_flag )
the_message[i] += 1;
}

// return the number of input characters used
return i;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    // Switch according to the ioctl called
    if( IOCTL_SET_MSGSLOT == ioctl_command_id )
    {
        // Get the parameter given to ioctl by the process
        printk( "Invoking ioctl: setting channel to %ld\n ", ioctl_param);
        if (ioctl_param<=0){
            return -EINVAL;
        }
        //setting the file->private_data to the channel id
        int index = file->private_data;
        channel_node *temp = slots[index]->first_channel;
        while (temp != NULL){
            //TODO: go over all the channels in the slot. change the curr channel if the id is matching, create one otherwise
        }
        // = (void *)ioctl_param;
        return SUCCESS;
    }

    return -EINVAL;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
        {
                .read           = device_read,
                .write          = device_write,
                .open           = device_open,
                .unlocked_ioctl = device_ioctl,
                .release        = device_release,
        };

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
    int rc = -1;
    slots=kmalloc(256*sizeof(message_slot));

    // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if( rc < 0 )
    {
        printk( KERN_ALERT "%s registraion failed for  %d\n",
                DEVICE_FILE_NAME, MAJOR_NUM );
        return rc;
    }

    printk(KERN_INFO "message slot registered successfully with major num%d\n ", MAJOR_NUM);

    return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    // Unregister the device
    // Should always succeed
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
