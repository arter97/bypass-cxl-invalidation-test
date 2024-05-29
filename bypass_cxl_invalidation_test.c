// SPDX-License-Identifier: GPL-2.0-only

/*
 * This kernel module utilizes kretprobe to make
 * arch/x86/mm/pat/set_memory.c:cpu_cache_has_invalidate_memregion() to
 * always return true.
 *
 * cpu_cache_has_invalidate_memregion() is used by
 * drivers/cxl/core/region.c:cxl_region_invalidate_memregion() to ensure
 * CPU architectural support of cache invalidation towards memory region.
 *
 * This kernel module can be used as an alternative when you wish to use
 * an unmodified kernel with CONFIG_CXL_REGION_INVALIDATION_TEST disabled.
 */

#define pr_fmt(fmt) "bypass-cxl-invalidation-test: " fmt

#include <linux/module.h>
#include <linux/kprobes.h>

static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	// Change return value to 1 (true)
	regs->ax = 1;
	return 0;
}
NOKPROBE_SYMBOL(ret_handler);

static struct kretprobe krp = {
	.handler		= ret_handler,
	.maxactive		= 1,
};

static int __init kretprobe_init(void)
{
	int ret;

	krp.kp.symbol_name = "cpu_cache_has_invalidate_memregion";
	ret = register_kretprobe(&krp);
	if (ret < 0) {
		pr_err("register_kretprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_info("kretprobe injected at %s: %p\n", krp.kp.symbol_name, krp.kp.addr);
	return 0;
}

static void __exit kretprobe_exit(void)
{
	unregister_kretprobe(&krp);
	pr_info("kretprobe at %p unregistered\n", krp.kp.addr);

	if (krp.nmissed)
		pr_err("missed probing %d instances of %s\n", krp.nmissed, krp.kp.symbol_name);
}

module_init(kretprobe_init)
module_exit(kretprobe_exit)
MODULE_LICENSE("GPL");
