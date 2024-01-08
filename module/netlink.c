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
    CREMONA_ATTR_CMD,
    CREMONA_ATTR_TOOT_ID,
    CREMONA_ATTR_DATA,
    __CREMONA_ATTR_AFTER_LAST,
};
#define CREMONA_A_MAX (__CREMONA_ATTR_AFTER_LAST - 1)
static struct nla_policy cremona_nla_policy[CREMONA_A_MAX + 1] = {
    [CREMONA_ATTR_NAME] = {.type = NLA_NUL_STRING},
    [CREMONA_ATTR_RESULT] = {.type = NLA_U32},
    [CREMONA_ATTR_DATA] = {.type = NLA_BINARY, .len = 1024},
};

/* commands */
enum cremona_commands
{
    CREMONA_CMD_UNSPEC,
    CREMONA_CMD_CONNECT,
    CREMONA_CMD_NOTIFY_ADD_DATA,
    CREMONA_CMD_READ_BUFFER,
    CREMONA_CMD_MOVE_NEXT,
    CREMONA_CMD_DISSCONNECT,
    __CREMONA_CMD_AFTER_LAST
};
#define CREMONA_C_MAX (__CREMONA_CMD_AFTER_LAST - 1)
static int cremona_connect(struct sk_buff *skb, struct genl_info *info);
static int netlink_cremona_read_buffer(struct sk_buff *skb, struct genl_info *info);
static int netlink_cremona_move_next_buffer(struct sk_buff *skb, struct genl_info *info);
static int netlink_cremona_disconnect(struct sk_buff *skb, struct genl_info *info);
static const struct genl_small_ops cremona_genl_ops[] = {
    {
        .cmd = CREMONA_CMD_CONNECT,
        .flags = 0,
        .doit = cremona_connect,
        .dumpit = NULL,
    },
    {
        .cmd = CREMONA_CMD_READ_BUFFER,
        .flags = 0,
        .doit = netlink_cremona_read_buffer,
        .dumpit = NULL,

    },
    {
        .cmd = CREMONA_CMD_MOVE_NEXT,
        .flags = 0,
        .doit = netlink_cremona_move_next_buffer,
        .dumpit = NULL,

    },
    {
        .cmd = CREMONA_CMD_DISSCONNECT,
        .flags = 0,
        .doit = netlink_cremona_disconnect,
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
    crmna_pr_info("error occurred while registering Cremona family!\n");
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
        crmna_pr_info("error %s with faild new genlmsg", __FUNCTION__);
        goto out_putrepeater;
    }
    reply_head = genlmsg_put_reply(reply, info, &cremona_genl_family, 0, info->genlhdr->cmd);

    if (!reply_head)
    {
        crmna_pr_info("error %s with faild put head", __FUNCTION__);
        goto out_free;
    }

    rc = nla_put_u32(reply, CREMONA_ATTR_RESULT, repeater ? 0 : -1);
    genlmsg_end(reply, reply_head);

    if (rc < 0)
    {
        crmna_pr_info("error %s with faild put CREMONA_ATTR_RESULT", __FUNCTION__);
        goto out_free;
    }

    return genlmsg_reply(reply, info);

out_free:
    nlmsg_free(reply);
out_putrepeater:
    repeater_put(repeater);
    return rc;
}

void netlink_send_add_data_message(Repeater *repeater)
{
    struct sk_buff *skb;
    void *msg_head;
    int rc = -ENOBUFS;

    skb = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
    if (!skb)
    {
        rc = -ENOMEM;
        crmna_pr_info("error %s with faild new genlmsg", __FUNCTION__);
        goto err_new;
    }
    msg_head = genlmsg_put(skb, 0, 0, &cremona_genl_family, 0, CREMONA_CMD_NOTIFY_ADD_DATA);
    if (!msg_head)
    {
        crmna_pr_info("error %s with faild put head", __FUNCTION__);
        goto out_free;
    }

    rc = nla_put_u32(skb, CREMONA_ATTR_RESULT, 0);
    genlmsg_end(skb, msg_head);

    if (rc < 0)
    {
        crmna_pr_info("error %s with faild put CREMONA_ATTR_RESULT", __FUNCTION__);
        goto out_free;
    }

    genlmsg_unicast(&init_net, skb, repeater_get_pid(repeater));
    return;

out_free:
    nlmsg_free(skb);
err_new:
    return;
}

static int reader(CREMONA_COMMAND_TYPE type, int id, const char *data, int data_len, void *context)
{
    int rc = -ENOBUFS;
    struct sk_buff *reply = (struct sk_buff *)context;

    rc = nla_put_u8(reply, CREMONA_ATTR_CMD, type);
    if (rc < 0)
    {
        crmna_pr_info("error %s with faild put CREMONA_ATTR_CMD rc: %d", __FUNCTION__, rc);
        return rc;
    }

    rc = nla_put_u32(reply, CREMONA_ATTR_TOOT_ID, id);
    if (rc < 0)
    {
        crmna_pr_info("error %s with faild put CREMONA_ATTR_TOOT_ID rc: %d", __FUNCTION__, rc);
        return rc;
    }

    rc = nla_put(reply, CREMONA_ATTR_DATA, data_len, data);
       if (rc < 0)
    {
        crmna_pr_info("error %s with faild put CREMONA_ATTR_DATA rc: %d", __FUNCTION__, rc);
        return rc;
    }
    return rc;
}

static int netlink_cremona_read_buffer(struct sk_buff *skb, struct genl_info *info)
{
    struct sk_buff *reply;
    void *reply_head;
    int rc = -ENOBUFS;

    reply = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
    if (!reply)
    {
        rc = -ENOMEM;
        crmna_pr_info("error %s with faild new genlmsg", __FUNCTION__);
        return rc;
    }
    reply_head = genlmsg_put_reply(reply, info, &cremona_genl_family, 0, info->genlhdr->cmd);

    if (!reply_head)
    {
        crmna_pr_info("error %s with faild put head", __FUNCTION__);
        goto out_free;
    }
    rc = cremona_read_buffer(cremona_instance, info->snd_portid, reader, reply);
    genlmsg_end(reply, reply_head);

    if (rc < 0)
    {
        crmna_pr_info("error %s with faild call reader", __FUNCTION__);
        goto out_free;
    }

    return genlmsg_reply(reply, info);

out_free:
    nlmsg_free(reply);
    return rc;
}

static int netlink_cremona_move_next_buffer(struct sk_buff *skb, struct genl_info *info)
{
    struct sk_buff *reply;
    void *reply_head;
    int rc = -ENOBUFS;

    reply = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
    if (!reply)
    {
        rc = -ENOMEM;
        crmna_pr_info("error %s with faild new genlmsg", __FUNCTION__);
        return rc;
    }
    reply_head = genlmsg_put_reply(reply, info, &cremona_genl_family, 0, info->genlhdr->cmd);

    if (!reply_head)
    {
        crmna_pr_info("error %s with faild put head", __FUNCTION__);
        goto out_free;
    }

    rc = nla_put_u32(reply, CREMONA_ATTR_RESULT, cremona_move_next_buffer(cremona_instance, info->snd_portid));
    genlmsg_end(reply, reply_head);

    if (rc < 0)
    {
        crmna_pr_info("error %s with faild put CREMONA_ATTR_RESULT", __FUNCTION__);
        goto out_free;
    }

    return genlmsg_reply(reply, info);

out_free:
    nlmsg_free(reply);
    return rc;
}

static int netlink_cremona_disconnect(struct sk_buff *skb, struct genl_info *info)
{
    struct sk_buff *reply;
    void *reply_head;
    int rc = -ENOBUFS;
    int result;

    reply = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
    if (!reply)
    {
        rc = -ENOMEM;
        crmna_pr_info("error %s with faild new genlmsg", __FUNCTION__);
        return rc;
    }
    reply_head = genlmsg_put_reply(reply, info, &cremona_genl_family, 0, info->genlhdr->cmd);
    if (!reply_head)
    {
        crmna_pr_info("error %s with faild put head", __FUNCTION__);
        goto out_free;
    }

    result = cremona_remove_repertor(cremona_instance, info->snd_portid);

    rc = nla_put_u32(reply, CREMONA_ATTR_RESULT, 0);
    genlmsg_end(reply, reply_head);

    if (rc < 0)
    {
        crmna_pr_info("error %s with faild put CREMONA_ATTR_RESULT", __FUNCTION__);
        goto out_free;
    }

    return genlmsg_reply(reply, info);

out_free:
    nlmsg_free(reply);
    return rc;
}