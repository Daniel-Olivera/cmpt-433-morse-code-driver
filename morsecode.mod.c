#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

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
	{ 0x5d0b370a, "module_layout" },
	{ 0x2c6a384a, "led_trigger_unregister_simple" },
	{ 0x36374531, "misc_deregister" },
	{ 0x7109916f, "led_trigger_register_simple" },
	{ 0x4c18a484, "misc_register" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xe914e41e, "strcpy" },
	{ 0xe2b4819f, "kmem_cache_alloc_trace" },
	{ 0xe021860a, "kmalloc_caches" },
	{ 0x16621f1f, "led_trigger_event" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x85df9b6c, "strsep" },
	{ 0x86332725, "__stack_chk_fail" },
	{ 0xf9a482f9, "msleep" },
	{ 0x11089ac7, "_ctype" },
	{ 0xae353d77, "arm_copy_from_user" },
	{ 0x51a910c0, "arm_copy_to_user" },
	{ 0x97255bdf, "strlen" },
	{ 0x2d6fcc06, "__kmalloc" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xc5850110, "printk" },
};

MODULE_INFO(depends, "");

