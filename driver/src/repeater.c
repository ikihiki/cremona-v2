#include <linux/kobject.h>
#include <linux/slab.h>

#include "cremona.h"

struct repeater_t
{
    struct kobject kobj;
    int pid;
    char name[CREMONA_MAX_DEVICE_NAME_LEN + 1];
};
#define to_repeater_obj(x) container_of(x, struct repeater_t, kobj)

struct repeater_attribute
{
    struct attribute attr;
    ssize_t (*show)(struct repeater_t *repeater, struct repeater_attribute *attr, char *buf);
    ssize_t (*store)(struct repeater_t *repeater, struct repeater_attribute *attr, const char *buf, size_t count);
};
#define to_repeater_attr(x) container_of(x, struct repeater_attribute, attr)

static ssize_t repeater_attr_show(struct kobject *kobj,
                                  struct attribute *attr,
                                  char *buf)
{
    struct repeater_attribute *attribute;
    struct repeater_t *repeater;

    attribute = to_repeater_attr(attr);
    repeater = to_repeater_obj(kobj);

    if (!attribute->show)
        return -EIO;

    return attribute->show(repeater, attribute, buf);
}

static ssize_t repeater_attr_store(struct kobject *kobj,
                                   struct attribute *attr,
                                   const char *buf, size_t len)
{
    struct repeater_attribute *attribute;
    struct repeater_t *repeater;

    attribute = to_repeater_attr(attr);
    repeater = to_repeater_obj(kobj);

    if (!attribute->store)
        return -EIO;

    return attribute->store(repeater, attribute, buf, len);
}

static const struct sysfs_ops repeater_sysfs_ops = {
    .show = repeater_attr_show,
    .store = repeater_attr_store,
};

static void repeater_release(struct kobject *kobj)
{
    struct repeater_t *repeater;

    repeater = to_repeater_obj(kobj);
    kfree(repeater);
}

static ssize_t pid_show(struct repeater_t *repeater, struct repeater_attribute *attr,
                        char *buf)
{
    return sysfs_emit(buf, "%d\n", repeater_get_pid(repeater));
}

static struct repeater_attribute repeater_attribute_pid =
    __ATTR_RO(pid);

static ssize_t name_show(struct repeater_t *repeater, struct repeater_attribute *attr,
                         char *buf)
{
    return sysfs_emit(buf, "%s\n", repeater_get_name(repeater));
}

static struct repeater_attribute repeater_attribute_name =
    __ATTR_RO(name);

static struct attribute *repeater_default_attrs[] = {
    &repeater_attribute_pid.attr,
    &repeater_attribute_name.attr,
    NULL,
};
ATTRIBUTE_GROUPS(repeater_default);

static const struct kobj_type repeater_ktype = {
    .sysfs_ops = &repeater_sysfs_ops,
    .release = repeater_release,
    .default_groups = repeater_default_groups,
};

int repeater_get_pid(Repeater *repeater)
{
    return repeater->pid;
}

char *repeater_get_name(Repeater *repeater)
{
    return repeater->name;
}

Repeater *repeater_create_and_add(struct kset *parent, const int pid, const char *name)
{
    Repeater *repeater;
    int retval;

    repeater = kzalloc(sizeof(*repeater), GFP_KERNEL);
    if (!repeater)
    {
        return NULL;
    }

    repeater->kobj.kset = parent;
    repeater->pid = pid;
    strncpy(repeater->name, name, CREMONA_MAX_DEVICE_NAME_LEN);

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
