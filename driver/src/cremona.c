#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/bits.h>
#include <linux/bitfield.h>

#include "cremona.h"

#define DRIVER_NAME "Cremona-Mastdon"
/* このデバイスドライバで使うマイナー番号の開始番号と個数(=デバイス数) */
static const uint32_t MINOR_BASE = 0;
static const uint32_t MINOR_NUM = 33; /* マイナー番号は 0 ~ 1 */

struct cremona_t
{
    struct kobject kobj;
    struct kset *repeaters;
    dev_t dev;
    struct class *device_class;
    repeater_add_data_callback callback;
};
#define to_cremona_obj(x) container_of(x, struct cremona_t, kobj)

struct cremona_attribute
{
    struct attribute attr;
    ssize_t (*show)(struct cremona_t *cremona, struct cremona_attribute *attr, char *buf);
    ssize_t (*store)(struct cremona_t *cremona, struct cremona_attribute *attr, const char *buf, size_t count);
};
#define to_cremona_attr(x) container_of(x, struct cremona_attribute, attr)

static ssize_t cremona_attr_show(struct kobject *kobj,
                                 struct attribute *attr,
                                 char *buf)
{
    struct cremona_attribute *attribute;
    struct cremona_t *cremona;

    attribute = to_cremona_attr(attr);
    cremona = to_cremona_obj(kobj);

    if (!attribute->show)
        return -EIO;

    return attribute->show(cremona, attribute, buf);
}

static ssize_t cremona_attr_store(struct kobject *kobj,
                                  struct attribute *attr,
                                  const char *buf, size_t len)
{
    struct cremona_attribute *attribute;
    struct cremona_t *cremona;

    attribute = to_cremona_attr(attr);
    cremona = to_cremona_obj(kobj);

    if (!attribute->store)
        return -EIO;

    return attribute->store(cremona, attribute, buf, len);
}

static const struct sysfs_ops cremona_sysfs_ops = {
    .show = cremona_attr_show,
    .store = cremona_attr_store,
};

static void cremona_release(struct kobject *kobj)
{
    struct cremona_t *cremona;

    cremona = to_cremona_obj(kobj);

    if (cremona->device_class != NULL)
    {
        class_destroy(cremona->device_class);
    }
    if (cremona->dev != 0)
    {
        unregister_chrdev_region(cremona->dev, MINOR_NUM);
    }
    kfree(cremona);
}

ssize_t status_show(struct cremona_t *cremona, struct cremona_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "OK\n");
}

static struct cremona_attribute cremona_status_attribute =
    __ATTR_RO(status);

static struct attribute *cremona_default_attrs[] = {
    &cremona_status_attribute.attr,
    NULL,
};

ATTRIBUTE_GROUPS(cremona_default);

static const struct kobj_type cremona_ktype = {
    .sysfs_ops = &cremona_sysfs_ops,
    .release = cremona_release,
    .default_groups = cremona_default_groups,
};

Cremona *cremona_create(repeater_add_data_callback callback)
{
    Cremona *instance;
    int ret;
    struct class *device_class;
    instance = (Cremona *)kzalloc(sizeof(Cremona), GFP_KERNEL);
    ret = kobject_init_and_add(&instance->kobj, &cremona_ktype, kernel_kobj, "cremona");
    if (ret != 0)
    {
        goto instance_error;
    }

    int alloc_ret =
        alloc_chrdev_region(&instance->dev, MINOR_BASE, MINOR_NUM, DRIVER_NAME);
    if (alloc_ret != 0)
    {
        pr_err("alloc_chrdev_region = %d\n", alloc_ret);
        goto instance_error;
    }

    device_class = class_create("cremona");
    if (IS_ERR(device_class))
    {
        pr_err("failed create device class \n");
        goto instance_error;
    }
    instance->device_class = device_class;

    instance->repeaters = kset_create_and_add("repeaters", NULL, &instance->kobj);
    if (!instance->repeaters)
    {
        goto repeaters_error;
    }
    instance->callback = callback;

    kobject_uevent(&instance->kobj, KOBJ_ADD);
    return instance;

repeaters_error:
instance_error:
    cremona_put(instance);
    return NULL;
}

Cremona *cremona_get(Cremona *cremona)
{
    kobject_get(&cremona->kobj);
    return cremona;
}

void cremona_put(Cremona *cremona)
{
    kobject_put(&cremona->kobj);
}

static Repeater* get_repeater_by_pid(Cremona *cremona, const int pid){
    char buf[12];
    snprintf(buf, 12, "%d", pid);
    struct kobject *repertorKobject;
    repertorKobject = kset_find_obj(cremona->repeaters, buf);
    if (repertorKobject == NULL)
    {
        return NULL;
    }
    return kobj2repeater(repertorKobject);
}

Repeater *cremona_add_repertor(Cremona *cremona, const int pid, const char *name)
{
    char buf[12];
    struct kobject *repertorKobject;
    snprintf(buf, 12, "%d", pid);

    repertorKobject = kset_find_obj(cremona->repeaters, buf);
    if (repertorKobject != NULL)
    {
        return NULL;
    }

    struct kobject *k;
    int dev_minor;
    Repeater *repeater;
    unsigned long long mask = 0;
    spin_lock(&cremona->repeaters->list_lock);
    list_for_each_entry(k, &cremona->repeaters->list, entry)
    {
        repeater = kobj2repeater(k);
        mask = mask | (1 << get_dev_minor(repeater));
    }
    spin_unlock(&cremona->repeaters->list_lock);

    for (int i = 0; i < 64; i++)
    {
        if ((mask & (1 << i)) == 0)
        {
            dev_minor = i;
            break;
        }
    }

    Repeater *repertor = repeater_create_and_add(cremona->repeaters, pid, name, MKDEV(MAJOR(cremona->dev), dev_minor), cremona->device_class, cremona->callback);

    return repertor;
}

int cremona_read_buffer(Cremona *cremona, const int pid, buffer_reader reader, void *data){
    Repeater *repeater = get_repeater_by_pid(cremona, pid);
    if(repeater == NULL){
        return -ENOENT;
    }
    return repeater_read_data(repeater, reader, data);
}
int cremona_move_next_buffer(Cremona *cremona, const int pid){
    Repeater *repeater = get_repeater_by_pid(cremona, pid);
    if(repeater == NULL){
        return -ENOENT;
    }
    return repeater_pop_data(repeater);
}

int cremona_remove_repertor(Cremona *cremona, const int pid){
    Repeater *repeater = get_repeater_by_pid(cremona, pid);
    if (repeater == NULL)
    {
        return -ENOENT;
    }

    repeater_put(repeater);
    return 0;
}
