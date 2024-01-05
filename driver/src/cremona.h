#ifndef CREMONA_HEADER
#define CREMONA_HEADER

typedef struct cremona_t Cremona;
typedef struct repeater_t Repeater;
typedef struct command_buffer_t CommandBuffer;

#define CREMONA_MAX_DEVICE_NAME_LEN 32

#define CIRCULAR_BUFFER_DATA_LEN 256
#define CIRCULAR_BUFFER_LEN 256

    typedef enum {
        CREMONA_REPERTER_DATA_TYPE_TOOT_CREATE,
        CREMONA_REPERTER_DATA_TYPE_TOOT_ADD_STRING,
        CREMONA_REPERTER_DATA_TYPE_TOOT_SEND
    } CREMONA_REPERTER_DATA_TYPE;

typedef struct circular_buffer_item_t{
CREMONA_REPERTER_DATA_TYPE type;
int id;
char data[CIRCULAR_BUFFER_DATA_LEN];
int data_len;
} CicularBufferItem;

typedef void (*repeater_add_data_callback)(Repeater *);
typedef int (*buffer_reader)(CicularBufferItem *item, void *data);

/////////////////////////////////////////////
/// Cremona
/////////////////////////////////////////////

/// @brief create a cremona instance
/// @return cremona instance if success, EINVAL if failed
Cremona *cremona_create(repeater_add_data_callback callback);
/// @brief increase cremona reference count
/// @param cremona cremona instance
/// @return cremona instance
Cremona *cremona_get(Cremona *cremona);
/// @brief decrease cremona reference count
/// @param cremona cremona instance
void cremona_put(Cremona *cremona);
/// @brief add a repeater to cremona
/// @param cremona cremona instance
/// @param pid repeater pid
/// @param name repeater device name
/// @return repeater instance if success, NULL if failed
Repeater* cremona_add_repertor(Cremona *cremona, const int pid, const char* name);
int cremona_read_buffer(Cremona *cremona, const int pid, buffer_reader reader, void *data);
int cremona_move_next_buffer(Cremona *cremona, const int pid);
int cremona_remove_repertor(Cremona *cremona, const int pid);

/////////////////////////////////////////////
/// Repeater
/////////////////////////////////////////////
/// @brief create a repeater instance and add it to parent
/// @param parent parent kset
/// @param pid repeater pid
/// @return repeater instance if success, NULL if failed
Repeater *repeater_create_and_add(struct kset *parent, const int pid, const char *name, dev_t dev, struct class *device_class, repeater_add_data_callback callback);
/// @brief destroy a repeater instance
/// @param repeater repeater instance
void repeater_put(Repeater *repeater);
/// @brief get repeater pid
/// @param repeater repeater instance
/// @return repeater pid
int repeater_get_pid(Repeater *repeater);
char *repeater_get_name(Repeater *repeater);

int repeater_read_data(Repeater *repeater, buffer_reader reader, void *data);
int repeater_pop_data(Repeater *repeater);
Repeater *kobj2repeater(struct kobject *x);
int get_dev_minor(Repeater *repeater);

/////////////////////////////////////////////
/// netlink
/////////////////////////////////////////////
int netlink_init(Cremona *cremona);
void netlink_destroy(void);
void netlink_send_add_data_message(Repeater *repeater);

/////////////////////////////////////////////
/// Command Buffer
/////////////////////////////////////////////
void command_buffer_create_toot(CommandBuffer *command_buffer, int id);
void command_buffer_send_toot(CommandBuffer *command_buffer, int id);
ssize_t command_buffer_add_str_from_usr(CommandBuffer *command_buffer, int id, const char __user *buf, size_t count);

#endif