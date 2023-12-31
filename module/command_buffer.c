#include <linux/spinlock.h>
#include <linux/circ_buf.h>
#include <linux/fs.h>

#include "cremona.h"

typedef struct command_t
{
    unsigned long time;
    CREMONA_COMMAND_TYPE type;
    int id;
    char data[CIRCULAR_BUFFER_DATA_LEN];
    int data_len;
} Command;

struct command_buffer_t
{
    int buffer_head;
    int buffer_tail;
    spinlock_t producer_lock;
    spinlock_t consumer_lock;
    Command *cicular_buffer;
};

CommandBuffer *create_command_buffer()
{
    CommandBuffer *command_buffer;
    command_buffer = kzalloc(sizeof(*command_buffer) + (sizeof(Command) * CIRCULAR_BUFFER_LEN), GFP_KERNEL);
    if (!command_buffer)
    {
        goto alloc_err;
    }

    command_buffer->cicular_buffer = (Command *)((void *)command_buffer + sizeof(*command_buffer));
    spin_lock_init(&command_buffer->producer_lock);
    spin_lock_init(&command_buffer->consumer_lock);

    return command_buffer;

alloc_err:
    return NULL;
}

void destroy_command_buffer(CommandBuffer *command_buffer)
{
    kfree(command_buffer);
}

static void command_buffer_add_data(CommandBuffer *command_buffer, CREMONA_COMMAND_TYPE type, int id, const char *data, int data_len)
{
    spin_lock(&command_buffer->producer_lock);
    int head = command_buffer->buffer_head;
    int tail = READ_ONCE(command_buffer->buffer_tail);
    int len = min(data_len, CIRCULAR_BUFFER_DATA_LEN);

    if (CIRC_SPACE(head, tail, CIRCULAR_BUFFER_LEN) >= 1)
    {
        Command *item = &command_buffer->cicular_buffer[head];
        item->time = jiffies;
        item->type = type;
        item->id = id;
        if (data != NULL)
        {
            memcpy(&item->data, data, len);
            item->data_len = len;
        }
        smp_store_release(&command_buffer->buffer_head, (head + 1) & (CIRCULAR_BUFFER_LEN - 1));
    }

    spin_unlock(&command_buffer->producer_lock);
}

static ssize_t command_buffer_add_data_user(CommandBuffer *command_buffer, CREMONA_COMMAND_TYPE type, int id, const char __user *buf, size_t count)
{
    spin_lock(&command_buffer->producer_lock);
    int head = command_buffer->buffer_head;
    int tail = READ_ONCE(command_buffer->buffer_tail);
    int len = min(count, (size_t)CIRCULAR_BUFFER_DATA_LEN);
    int rc;

    if (CIRC_SPACE(head, tail, CIRCULAR_BUFFER_LEN) >= 1)
    {
        Command *item = &command_buffer->cicular_buffer[head];
        item->time = jiffies;
        item->type = type;
        item->id = id;
        if (buf != NULL)
        {
            if (copy_from_user(&item->data, buf, len) != 0)
            {
                item->data_len = 0;
                rc = -EFAULT;
            }
            else
            {
                item->data_len = len;
                rc = len;
            }
        }
        else
        {
            item->data_len = 0;
            rc = 0;
        }

        smp_store_release(&command_buffer->buffer_head, (head + 1) & (CIRCULAR_BUFFER_LEN - 1));
    }

    spin_unlock(&command_buffer->producer_lock);
    return rc;
}

int command_buffer_read_data(CommandBuffer *command_buffer, buffer_reader reader, void *context)
{
    int rc = 0;
    spin_lock(&command_buffer->consumer_lock);
    int head = smp_load_acquire(&command_buffer->buffer_head);
    int tail = command_buffer->buffer_tail;
    if (CIRC_CNT(head, tail, CIRCULAR_BUFFER_LEN) >= 1)
    {
        Command *item = &command_buffer->cicular_buffer[tail];
        rc = reader(item->type, item->id, item->data, item->data_len, context);
    }

    spin_unlock(&command_buffer->consumer_lock);
    return rc;
}

int command_buffer_pop_data(CommandBuffer *command_buffer)
{
    int rc;
    spin_lock(&command_buffer->consumer_lock);
    int head = smp_load_acquire(&command_buffer->buffer_head);
    int tail = command_buffer->buffer_tail;

    if (CIRC_CNT(head, tail, CIRCULAR_BUFFER_LEN) >= 1)
    {
        smp_store_release(&command_buffer->buffer_tail, (tail + 1) & (CIRCULAR_BUFFER_LEN - 1));
    }

    spin_unlock(&command_buffer->consumer_lock);

    rc = command_buffer_get_count(command_buffer);
    return rc;
}

void command_buffer_create_toot(CommandBuffer *command_buffer, int id)
{
    command_buffer_add_data(command_buffer, CREMONA_COMMAND_TYPE_CREATE_TOOT, id, NULL, 0);
}

void command_buffer_send_toot(CommandBuffer *command_buffer, int id)
{
    command_buffer_add_data(command_buffer, CREMONA_COMMAND_TYPE_SEND_TOOT, id, NULL, 0);
}

ssize_t command_buffer_add_str_from_usr(CommandBuffer *command_buffer, int id, const char __user *buf, size_t count)
{
    return command_buffer_add_data_user(command_buffer, CREMONA_COMMAND_TYPE_TOOT_ADD_STRING, id, buf, count);
}

int command_buffer_get_count(CommandBuffer *command_buffer)
{
    int head = smp_load_acquire(&command_buffer->buffer_head);
    int tail = command_buffer->buffer_tail;

    return CIRC_CNT(head, tail, CIRCULAR_BUFFER_LEN);
}

int command_buffer_add_keep_alive(CommandBuffer *command_buffer)
{
    bool isEmpty = false;
    bool isHang = false;

    spin_lock(&command_buffer->consumer_lock);
    int head = smp_load_acquire(&command_buffer->buffer_head);
    int tail = command_buffer->buffer_tail;
    if (CIRC_CNT(head, tail, CIRCULAR_BUFFER_LEN) >= 1)
    {
        Command *item = &command_buffer->cicular_buffer[tail];
        if (item->time + 10000 < jiffies)
        {
            isHang = true;
        }
    }
    else
    {
        isEmpty = true;
    }

    spin_unlock(&command_buffer->consumer_lock);

    if (isEmpty)
    {
        command_buffer_add_data(command_buffer, CREMONA_COMMAND_TYPE_KEEP_ALIVE, 0, NULL, 0);
    }
    return isHang ? -1 : isEmpty ? 1 : 0;
}