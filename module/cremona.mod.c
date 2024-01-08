#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x656e4a6e, "snprintf" },
	{ 0xbcf4fea, "class_destroy" },
	{ 0xc533544c, "genlmsg_put" },
	{ 0x37a0cba, "kfree" },
	{ 0x860e28d9, "kernel_kobj" },
	{ 0xdc0e4855, "timer_delete" },
	{ 0xe783e261, "sysfs_emit" },
	{ 0x122c3a7e, "_printk" },
	{ 0x89ecab6e, "__alloc_skb" },
	{ 0x63ec3b68, "cdev_add" },
	{ 0x76794a38, "init_net" },
	{ 0x24d273d1, "add_timer" },
	{ 0x28ec5d6c, "netlink_unicast" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0xea669cdc, "device_create" },
	{ 0x15b33ba8, "class_create" },
	{ 0x2985870d, "kfree_skb_reason" },
	{ 0xb0933302, "nla_put" },
	{ 0x9166fada, "strncpy" },
	{ 0xf67c89c5, "kobject_init_and_add" },
	{ 0x9ed12e20, "kmalloc_large" },
	{ 0xda02d67, "jiffies" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x17635dea, "kset_find_obj" },
	{ 0x513445a2, "device_destroy" },
	{ 0x2787ba1c, "kobject_uevent" },
	{ 0x84823cf3, "nla_strscpy" },
	{ 0x3def6938, "genl_unregister_family" },
	{ 0xa05e33b, "kmalloc_trace" },
	{ 0x80571b54, "genl_register_family" },
	{ 0x413dd120, "cdev_init" },
	{ 0x2f837887, "kobject_get" },
	{ 0x7691b6c, "kmalloc_caches" },
	{ 0x1b5bbd74, "cdev_del" },
	{ 0x3ddbf5df, "kset_create_and_add" },
	{ 0x6d319229, "kobject_put" },
	{ 0xfe22f55c, "module_layout" },
};

MODULE_INFO(depends, "");

