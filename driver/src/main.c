#include <linux/init.h>
#include <linux/module.h>

#include "cremona.h"

Cremona *instance;
Repeater *repeater;


MODULE_LICENSE("Dual BSD/GPL");

void repeater_callback(Repeater * repeater){
    netlink_send_add_data_message(repeater);
}

static int hello_init(void)
{
    int rc;
    printk(KERN_ALERT "Hello, world\n");
    instance = cremona_create(&repeater_callback);
    if(!instance)
    {
        printk(KERN_ALERT "cremona_create failed\n");
        return -EINVAL;
    }

    repeater  = cremona_add_repertor(instance, 1, "test");
    if(!repeater)
    {
        printk(KERN_ALERT "cremona_add_repertor failed\n");
        cremona_put(instance);
        return -EINVAL;
    }

    rc = netlink_init(instance);
    if(rc != 0)
    {
        printk(KERN_ALERT "netlink_init failed\n");
        repeater_put(repeater);
        cremona_put(instance);
        return -EINVAL;
    }

    return 0;
}
static void hello_exit(void)
{
    netlink_destroy();
    repeater_put(repeater);
    cremona_put(instance);
    printk(KERN_ALERT "Goodbye, cruel world\n");
}
module_init(hello_init);
module_exit(hello_exit);