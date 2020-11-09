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
    unsigned long channel_id;
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
static int minor;

//==================== HELPER FUNCTIONS =============================

void free_channels_list(channel_node *head){
    channel_node *temp;
    while (head!=NULL){
        temp = head;
        head = head->next_channel;
        kfree(temp);
    }
}

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
    printk("invokig device_open(%p)\n", file);
    minor = (int)iminor(inode);
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
static int device_release( struct inode* inode, struct file*  file)
{
    printk("Invoking device_release(%p,%p)\n", inode, file);
    minor = -1;
    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file, char __user* buffer, size_t length, loff_t* offset )
{
    char *channel_text;
    printk( "Invoking device_read(%p,%ld)\n", file, length );
    if(file->private_data == NULL) {
        return -EINVAL;
    }
    channel_node *curr_channel = slots[minor]->curr_channel;
    if (curr_channel != NULL){
        if (length < curr_channel->text_length || buffer == NULL){
            return -ENOSPC;
        }
        channel_text = kmalloc(sizeof(char)*curr_channel->text_length, GFP_KERNEL);
        if (channel_text == NULL){
            return -ENOSPC;
        }
        memcpy(channel_text, curr_channel->buffer, curr_channel->text_length);
        if (copy_to_user(buffer, channel_text, curr_channel->text_length)!=SUCCESS){
            return -ENOSPC;
        }
        kfree(channel_text);
        return curr_channel->text_length;
    }
    return -EWOULDBLOCK;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file* file, const char __user* buffer,size_t length, loff_t* offset)
{
    char *user_text;
    printk("Invoking device_write(%p,%ld)\n", file, length);
    if (file ->private_data == NULL){
        return -EINVAL;
    }
    if (length <=0 || length >BUF_LEN ){
        return -EMSGSIZE;
    }
    if (buffer == NULL){
        return -ENOSPC;
    }
    channel_node *curr_channel = slots[minor]->curr_channel;
    curr_channel->text_length = length;
    user_text = kmalloc(sizeof(char) * length, GFP_KERNEL);
    if (user_text == NULL){
        return -ENOSPC;
    }
    if (copy_from_user(user_text, buffer, length)!=0){
        printk("failed to copy all the %ld chars", length);
        return -ENOSPC;
    }
    memcpy(curr_channel->buffer, user_text, length);
    curr_channel->text_length = length;
    kfree(user_text);
    return length;



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
        //setting the slots[minor] to the  relevant channel
        file->private_data = (void *) ioctl_param;
        channel_node *temp = slots[minor]->first_channel;
        while (temp != NULL){
            if (temp->channel_id==ioctl_param){
                slots[minor]->curr_channel = temp;
                return SUCCESS;
            }
        }
        temp = (channel_node *)kmalloc(sizeof(channel_node), GFP_KERNEL);
        if (temp == NULL){
            printk("kmalloc error\n");
            return -EINVAL;
        }
        temp->channel_id = ioctl_param;
        temp->next_channel = slots[minor]->first_channel;
        slots[minor]->curr_channel = temp;
        slots[minor]->first_channel = temp;
        return SUCCESS;
    }
    printk("invalid ioctl command:%ld\n", ioctl_param);
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
    if (slots == NULL){
        printk("kmalloc error\n");
        return -EINVAL;
    }

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
    printk("exiting device nubmer %d", minor);
    for(int i=0; i<TOTAL_DEVICES; i++){
        free_channels_list(slots[i]->first_channel);
        kfree(slots[i]);
    }
    // Unregister the device
    // Should always succeed
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
