#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#include "cremona.h"

struct cremona_t
{
    struct kobject kobj;
    struct kset *repeaters;
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

Cremona *cremona_create(void)
{
    Cremona *instance;
    int ret;
    instance = (Cremona *)kzalloc(sizeof(Cremona), GFP_KERNEL);
    ret = kobject_init_and_add(&instance->kobj, &cremona_ktype, kernel_kobj, "cremona");
    if (ret != 0)
    {
        goto instance_error;
    }

    instance->repeaters = kset_create_and_add("repeaters", NULL, &instance->kobj);
    if (!instance->repeaters)
    {
        goto repeaters_error;
    }

    kobject_uevent(&instance->kobj, KOBJ_ADD);
    return instance;

repeaters_error:
instance_error:
    cremona_put(instance);
    return NULL;
}

Cremona *cremona_get(Cremona *cremona){
    kobject_get(&cremona->kobj);
    return cremona;
}



void cremona_put(Cremona *cremona)
{
    kobject_put(&cremona->kobj);
}

Repeater* cremona_add_repertor(Cremona *cremona, const int pid, const char* name)
{
    char buf[12];
    struct kobject *repertorKobject;
    snprintf(buf, 12, "%d", pid);

    repertorKobject = kset_find_obj(cremona->repeaters, buf);
    if (repertorKobject != NULL)
    {
        return NULL;
    }

    Repeater *repertor = repeater_create_and_add(cremona->repeaters, pid, name);

    return repertor;
}