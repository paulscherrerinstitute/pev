#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
};

static const struct modversion_info ____versions[]
__attribute_used__
__attribute__((section("__versions"))) = {
	{ 0x89e24b9c, "struct_module" },
	{ 0x70ecc9b2, "cdev_del" },
	{ 0x4ac7b024, "pci_bus_read_config_byte" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0xd2248aac, "cdev_init" },
	{ 0xab978df6, "malloc_sizes" },
	{ 0xa03d6a57, "__get_user_4" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xd16ac615, "__get_user_1" },
	{ 0xcdb08c03, "pci_bus_write_config_word" },
	{ 0xda4008e6, "cond_resched" },
	{ 0x1b7d4074, "printk" },
	{ 0x2da418b5, "copy_to_user" },
	{ 0x56134363, "pci_bus_write_config_dword" },
	{ 0xc3aaf0a9, "__put_user_1" },
	{ 0x625acc81, "__down_failed_interruptible" },
	{ 0x9eac042a, "__ioremap" },
	{ 0x6ca08048, "pci_enable_msi" },
	{ 0x5e22fdec, "cdev_add" },
	{ 0x19070091, "kmem_cache_alloc" },
	{ 0xb2fd5ceb, "__put_user_4" },
	{ 0x78021fa2, "pci_bus_read_config_word" },
	{ 0x7561ed, "pci_bus_read_config_dword" },
	{ 0x107d6ba3, "__get_free_pages" },
	{ 0x26e96637, "request_irq" },
	{ 0x6b2dc060, "dump_stack" },
	{ 0x19cacd0, "init_waitqueue_head" },
	{ 0x9941ccb8, "free_pages" },
	{ 0xb5f46a8b, "pci_bus_write_config_byte" },
	{ 0x37a0cba, "kfree" },
	{ 0x2b6754b3, "remap_pfn_range" },
	{ 0x4a6e42f1, "pci_disable_msi" },
	{ 0xedc03953, "iounmap" },
	{ 0x5a4896a8, "__put_user_2" },
	{ 0x65ddd69, "pci_get_device" },
	{ 0x60a4461c, "__up_wakeup" },
	{ 0x4888a014, "__get_user_2" },
	{ 0xdd994dbd, "pci_enable_device" },
	{ 0xf2a644fb, "copy_from_user" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xf20dabd8, "free_irq" },
};

static const char __module_depends[]
__attribute_used__
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "9A5C9A113B5D63CDCD2BC7A");
