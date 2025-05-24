// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Alexander Bugaev 
 *
 * This file is released under the GPL.
 */

#include <linux/device-mapper.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>

#define DM_MSG_PREFIX "proxy"

/*
 * A struct containing the proxy target context
 */
struct dm_proxy_target {
	struct dm_dev *dev;

	// add stats here
};

/*
 * Create a proxy target over the given device 
 */
static int proxy_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	struct dm_proxy_target *dmpt;

	if (argc != 1) {
		ti->error = "Invalid argument count";
		return -EINVAL;
	}

	dmpt = (struct dm_proxy_target*)kmalloc(sizeof(struct dm_proxy_target), GFP_KERNEL);

	if (dmpt == NULL) {
		ti->error = "Failed to allocate proxy context";
		return -ENOMEM;
	}

	if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &dmpt->dev)) {
		ti->error = "Device lookup failed";
		kfree(dmpt);
		return -EINVAL;
	}

	ti->private = dmpt;
	return 0;
}

/*
 * Release the proxy target context
 */
static void proxy_dtr(struct dm_target *ti)
{
	struct dm_proxy_target *dmpt = ti->private;

	dm_put_device(ti, dmpt->dev);
	kfree(dmpt);
}

/*
 * Update stats and redirect bio to the underlying device
 */
static int proxy_map(struct dm_target *ti, struct bio *bio)
{
	return 0;
}

static struct target_type proxy_target = {
	.name   = "proxy",
	.version = {1, 0, 0},
	.features = DM_TARGET_NOWAIT,
	.module = THIS_MODULE,
	.ctr    = proxy_ctr,
	.dtr	= proxy_dtr,
	.map    = proxy_map,
};
module_dm(proxy);

MODULE_AUTHOR("Alexander Bugaev");
MODULE_DESCRIPTION(DM_NAME " target collecting request statistics");
MODULE_LICENSE("GPL");

