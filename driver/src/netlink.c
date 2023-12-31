#include <net/netlink.h>
#include <net/genetlink.h>
#include <linux/kobject.h>

#include "cremona.h"

static Cremona *cremona_instance;

enum cremona_netlink_attrs
{
    CREMONA_ATTR_UNSPEC,
    CREMONA_ATTR_NAME,
    CREMONA_ATTR_RESULT,
    CREMONA_ATTR_DATA,
    __CREMONA_ATTR_AFTER_LAST,
};
#define CREMONA_A_MAX (__CREMONA_ATTR_AFTER_LAST - 1)
static struct nla_policy cremona_nla_policy[CREMONA_A_MAX + 1] = {
    [CREMONA_ATTR_NAME] = {.type = NLA_NUL_STRING},
    [CREMONA_ATTR_RESULT] = {.type = NLA_U8},
    [CREMONA_ATTR_DATA] = {.type = NLA_BINARY, .len = 1024},
};

/* commands */
enum cremona_commands
{
    CREMONA_CMD_UNSPEC,
    CREMONA_CMD_CONNECT,
    __CREMONA_CMD_AFTER_LAST
};
#define CREMONA_C_MAX (__CREMONA_CMD_AFTER_LAST - 1)
static int cremona_connect(struct sk_buff *skb, struct genl_info *info);
static const struct genl_small_ops cremona_genl_ops[] = {
    {
        .cmd = CREMONA_CMD_CONNECT,
        .flags = 0,
        .doit = cremona_connect,
        .dumpit = NULL,

    }};
static struct genl_family cremona_genl_family = {
    .id = 0 /*GENL_ID_GENERATE*/,
    .hdrsize = 0,
    .name = "CREMONA",
    .version = 1,
    .maxattr = CREMONA_A_MAX,
    .policy = cremona_nla_policy,
    .small_ops = cremona_genl_ops,
    .n_small_ops = ARRAY_SIZE(cremona_genl_ops),
};

int netlink_init(Cremona *cremona)
{
    cremona_instance = cremona_get(cremona);

    int ret;
    ret = genl_register_family(&cremona_genl_family);
    if (ret != 0)
    {
        goto familly_error;
    }

    return 0;

familly_error:
    printk("error occurred while registering GENL_EXMPL family!\n");
    return ret;
}

void netlink_destroy(void)
{
    genl_unregister_family(&cremona_genl_family);
    cremona_put(cremona_instance);
    cremona_instance = NULL;
}

static int cremona_connect(struct sk_buff *skb, struct genl_info *info)
{
    struct sk_buff *reply;
    void *reply_head;
    Repeater *repeater;
    char name[CREMONA_MAX_DEVICE_NAME_LEN + 1];
    int rc = -ENOBUFS;

    if (!info->attrs[CREMONA_ATTR_NAME])
        return -EINVAL;

    nla_strscpy(name, info->attrs[CREMONA_ATTR_NAME], sizeof(name));

    repeater = cremona_add_repertor(cremona_instance, info->snd_portid, name);

    reply = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
    if (!reply)
    {
        rc = -ENOMEM;
        goto out_putrepeater;
    }
    reply_head = genlmsg_put_reply(reply, info, &cremona_genl_family, 0, CREMONA_CMD_CONNECT);
    if (!reply_head)
    {
        goto out_free;
    }

    rc = nla_put_u8(reply, CREMONA_ATTR_RESULT, repeater ? 0 : -1);
    genlmsg_end(reply, reply_head);

    if (rc < 0)
        goto out_free;

    return genlmsg_reply(reply, info);

out_free:
    nlmsg_free(reply);
out_putrepeater:
    repeater_put(repeater);
    return rc;
}