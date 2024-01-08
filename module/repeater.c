#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#include "cremona.h"

struct repeater_t
{
    struct kobject kobj;
    int pid;
    char name[CREMONA_MAX_DEVICE_NAME_LEN + 1];
    CommandBuffer *command_buffer;
    dev_t dev;
    struct cdev cdev;
    spinlock_t toot_count_lock;
    int toot_count;
    bool cdev_added;
    struct device *device;
    repeater_add_data_callback callback;
};

struct inode_data
{
    int id;
    Repeater *repeater;
};

Repeater *kobj2repeater(struct kobject *x)
{
    return container_of(x, struct repeater_t, kobj);
}

Repeater *repeater_get(Repeater *repeater)
{
    kobject_get(&repeater->kobj);
    return repeater;
}

#define cdev2repeater(x) container_of(x, struct repeater_t, cdev)

struct repeater_attribute
{
    struct attribute attr;
    ssize_t (*show)(struct repeater_t *repeater, struct repeater_attribute *attr, char *buf);
    ssize_t (*store)(struct repeater_t *repeater, struct repeater_attribute *attr, const char *buf, size_t count);
};
#define to_repeater_attr(x) container_of(x, struct repeater_attribute, attr)

static ssize_t repeater_attr_show(struct kobject *kobj,
                                  struct attribute *attr,
                                  char *buf);
static ssize_t repeater_attr_store(struct kobject *kobj,
                                   struct attribute *attr,
                                   const char *buf, size_t len);
static const struct sysfs_ops repeater_sysfs_ops = {
    .show = repeater_attr_show,
    .store = repeater_attr_store,
};

static ssize_t pid_show(struct repeater_t *repeater, struct repeater_attribute *attr,
                        char *buf);
static struct repeater_attribute repeater_attribute_pid =
    __ATTR_RO(pid);
static ssize_t name_show(struct repeater_t *repeater, struct repeater_attribute *attr,
                         char *buf);
static struct repeater_attribute repeater_attribute_name =
    __ATTR_RO(name);
static ssize_t buffer_count_show(struct repeater_t *repeater, struct repeater_attribute *attr,
                                 char *buf);
static struct repeater_attribute repeater_attribute_buffer_count =
    __ATTR_RO(buffer_count);
static struct attribute *repeater_default_attrs[] = {
    &repeater_attribute_pid.attr,
    &repeater_attribute_name.attr,
    &repeater_attribute_buffer_count.attr,
    NULL,
};
ATTRIBUTE_GROUPS(repeater_default);

static void repeater_release(struct kobject *kobj);
static const struct kobj_type repeater_ktype = {
    .sysfs_ops = &repeater_sysfs_ops,
    .release = repeater_release,
    .default_groups = repeater_default_groups,
};

static int repeater_device_open(struct inode *inode, struct file *filep);
static int repeater_device_release(struct inode *inode, struct file *filep);
static ssize_t repeater_device_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t repeater_device_write(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos);
struct file_operations repeater_fops = {
    .open = repeater_device_open,
    .read = repeater_device_read,
    .write = repeater_device_write,
    .release = repeater_device_release};

/////////////////////////////////////////////
/// Object Manipulation
/////////////////////////////////////////////
int repeater_get_pid(Repeater *repeater)
{
    return repeater->pid;
}

char *repeater_get_name(Repeater *repeater)
{
    return repeater->name;
}

int get_dev_minor(Repeater *repeater)
{
    return MINOR(repeater->dev);
}

static void repeater_destroy(Repeater *repeater)
{
    if (repeater->device)
    {
        device_destroy(repeater->device->class, repeater->dev);
    }
    if (repeater->cdev_added)
    {
        cdev_del(&repeater->cdev);
    }
    if (repeater->command_buffer != NULL)
    {
        destroy_command_buffer(repeater->command_buffer);
    }
    kfree(repeater);
}

Repeater *repeater_create_and_add(struct kset *parent, const int pid, const char *name, dev_t dev, struct class *device_class, repeater_add_data_callback callback)
{
    Repeater *repeater;
    int retval;
    int err;
    struct device *device = NULL;
    CommandBuffer *command_buffer = NULL;

    repeater = kzalloc(sizeof(*repeater), GFP_KERNEL);
    if (!repeater)
    {
        return NULL;
    }

    command_buffer = create_command_buffer();
    if (!command_buffer)
    {
        repeater_destroy(repeater);
        return NULL;
    }
    repeater->command_buffer = command_buffer;

    repeater->kobj.kset = parent;
    repeater->pid = pid;
    repeater->dev = dev;
    repeater->callback = callback;
    strncpy(repeater->name, name, CREMONA_MAX_DEVICE_NAME_LEN);
    spin_lock_init(&repeater->toot_count_lock);

    cdev_init(&repeater->cdev, &repeater_fops);
    repeater->cdev.owner = THIS_MODULE;
    retval = cdev_add(&repeater->cdev, repeater->dev, 1);
    if (retval)
    {
        repeater_destroy(repeater);
        return NULL;
    }
    repeater->cdev_added = true;

    device = device_create(device_class, NULL, dev, NULL, repeater->name);
    if (IS_ERR(device))
    {
        repeater_destroy(repeater);
        return NULL;
    }
    repeater->device = device;

    retval = kobject_init_and_add(&repeater->kobj, &repeater_ktype, NULL, "%d", pid);
    if (retval)
    {
        kobject_put(&repeater->kobj);
        return NULL;
    }

    /*
     * We are always responsible for sending the uevent that the kobject
     * was added to the system.
     */
    kobject_uevent(&repeater->kobj, KOBJ_ADD);

    return repeater;
}

void repeater_put(Repeater *repeater)
{
    if (repeater != NULL)
    {
        kobject_put(&repeater->kobj);
    }
}

/////////////////////////////////////////////
/// KObject Manipulation
/////////////////////////////////////////////
static ssize_t repeater_attr_store(struct kobject *kobj,
                                   struct attribute *attr,
                                   const char *buf, size_t len)
{
    struct repeater_attribute *attribute;
    struct repeater_t *repeater;

    attribute = to_repeater_attr(attr);
    repeater = kobj2repeater(kobj);

    if (!attribute->store)
        return -EIO;

    return attribute->store(repeater, attribute, buf, len);
}
static ssize_t repeater_attr_show(struct kobject *kobj,
                                  struct attribute *attr,
                                  char *buf)
{
    struct repeater_attribute *attribute;
    struct repeater_t *repeater;

    attribute = to_repeater_attr(attr);
    repeater = kobj2repeater(kobj);

    if (!attribute->show)
        return -EIO;

    return attribute->show(repeater, attribute, buf);
}

static void repeater_release(struct kobject *kobj)
{
    struct repeater_t *repeater;

    repeater = kobj2repeater(kobj);
    repeater_destroy(repeater);
}

/////////////////////////////////////////////
/// KObject Attribute
/////////////////////////////////////////////
static ssize_t pid_show(struct repeater_t *repeater, struct repeater_attribute *attr,
                        char *buf)
{
    return sysfs_emit(buf, "%d\n", repeater_get_pid(repeater));
}

static ssize_t name_show(struct repeater_t *repeater, struct repeater_attribute *attr,
                         char *buf)
{
    return sysfs_emit(buf, "%s\n", repeater_get_name(repeater));
}

static ssize_t buffer_count_show(struct repeater_t *repeater, struct repeater_attribute *attr,
                                 char *buf)
{
    return sysfs_emit(buf, "%d\n", command_buffer_get_count(repeater->command_buffer));
}

/////////////////////////////////////////////
/// Device Manipulation
/////////////////////////////////////////////
static int repeater_device_open(struct inode *inode, struct file *filep)
{
    Repeater *repeater;
    struct inode_data *data;
    int toot_id;
    repeater = cdev2repeater(inode->i_cdev);

    spin_lock(&repeater->toot_count_lock);
    toot_id = repeater->toot_count++;
    spin_unlock(&repeater->toot_count_lock);

    data = kzalloc(sizeof(struct inode_data), GFP_KERNEL);
    if (!data)
    {
        return -ENOMEM;
    }
    data->id = toot_id;
    data->repeater = repeater_get(repeater);
    filep->private_data = data;
    command_buffer_create_toot(repeater->command_buffer, toot_id);
    repeater->callback(repeater);

    return 0;
}

static int repeater_device_release(struct inode *inode, struct file *filep)
{
    struct inode_data *data;

    data = (struct inode_data *)filep->private_data;
    command_buffer_send_toot(data->repeater->command_buffer, data->id);
    data->repeater->callback(data->repeater);
    repeater_put(data->repeater);
    kfree(data);
    filep->private_data = NULL;

    return 0;
}
static ssize_t repeater_device_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos)
{
    return 0;
}
static ssize_t repeater_device_write(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos)
{

    struct inode_data *data;
    ssize_t rc;

    data = (struct inode_data *)filep->private_data;
    rc = command_buffer_add_str_from_usr(data->repeater->command_buffer, data->id, buf, count);
    data->repeater->callback(data->repeater);

    return rc;
}

/////////////////////////////////////////////
/// Netlink command handler
/////////////////////////////////////////////
int repeater_read_data(Repeater *repeater, buffer_reader reader, void *context)
{
    return command_buffer_read_data(repeater->command_buffer, reader, context);
}

int repeater_pop_data(Repeater *repeater)
{
    return command_buffer_pop_data(repeater->command_buffer);
}

/////////////////////////////////////////////
/// Timer Manipulation
/////////////////////////////////////////////
int repeater_add_keep_alive(Repeater *repeater)
{
    int retval;
    retval = command_buffer_add_keep_alive(repeater->command_buffer);
    if (retval < 0)
    {
        return -1;
    }
    else if (retval > 0)
    {
        repeater->callback(repeater);
    }

    return 0;
}