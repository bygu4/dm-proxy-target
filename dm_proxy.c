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
#include <linux/sysfs.h>

#define DM_MSG_PREFIX "proxy"
#define KOBJ_NAME "stat"

/*
 * A struct containing the block request statistics
 */
struct bio_stats {
	size_t total_size;
	unsigned int blk_count;
	unsigned int req_count;
};

static struct bio_stats r_stats = {};
static struct bio_stats w_stats = {};

static struct kobject *stats_kobj;

/*
 * Display stats using the corresponding attribute
 */
static ssize_t stats_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	size_t r_avg_block = r_stats.blk_count != 0 ? r_stats.total_size / r_stats.blk_count : 0;
	size_t w_avg_block = w_stats.blk_count != 0 ? w_stats.total_size / w_stats.blk_count : 0;
	size_t total_avg_block = r_stats.blk_count + w_stats.blk_count != 0
		? (r_stats.total_size + w_stats.total_size) / (r_stats.blk_count + w_stats.blk_count)
		: 0;
	return sprintf(
		buf,
		"read:\n\treqs: %u\n\tavg_block: %zu\nwrite:\n\treqs: %u\n\tavg_block: %zu\ntotal:\n\treqs: %u\n\tavg_block: %zu\n",
		r_stats.req_count,
		r_avg_block,
		w_stats.req_count,
		w_avg_block,
		r_stats.req_count + w_stats.req_count,
		total_avg_block
	);
}

static struct kobj_attribute stats_attr = __ATTR(volumes, 0444, stats_show, NULL);

/*
 * Create a proxy target over the given device 
 */
static int proxy_ctr(struct dm_target *ti, unsigned int argc, char **argv) {
	if (argc != 1) {
		ti->error = "Invalid argument count";
		return -EINVAL;
	}

	struct dm_dev *dev;

	if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &dev)) {
		ti->error = "Device lookup failed";
		return -EINVAL;
	}

	ti->private = dev;
	return 0;
}

/*
 * Release the proxy target context
 */
static void proxy_dtr(struct dm_target *ti) {

	struct dm_dev *dev = ti->private;
	dm_put_device(ti, dev);
}

/*
 * Update the given stats according to the given request
 */
static void update_stats(struct bio_stats *stats, struct bio *bio) {
	struct bio_vec bvec;
	struct bvec_iter iter;
	bio_for_each_segment(bvec, bio, iter) {
		stats->total_size += bvec.bv_len;
		++stats->blk_count;
	}
 
	++stats->req_count;
}

/*
 * Update stats and redirect bio to the underlying device
 */
static int proxy_map(struct dm_target *ti, struct bio *bio)
{
	struct dm_dev *dev = ti->private;

	if (bio_data_dir(bio) == WRITE)
		update_stats(&w_stats, bio);
	else
		update_stats(&r_stats, bio);

	bio_set_dev(bio, dev->bdev);
	submit_bio(bio);
	return DM_MAPIO_SUBMITTED;
}

static struct target_type dmpt = {
	.name   = "proxy",
	.version = {1, 0, 0},
	.module = THIS_MODULE,
	.ctr    = proxy_ctr,
	.dtr	= proxy_dtr,
	.map    = proxy_map,
};

/*
 * Register the target and create sysfs on init
 */
static int __init proxy_init(void) {
	stats_kobj = kobject_create_and_add(KOBJ_NAME, &THIS_MODULE->mkobj.kobj);

	if (stats_kobj == NULL) {
		DMERR("Failed to create kobject");
		return -ENOMEM;
	}

	if (sysfs_create_file(stats_kobj, &stats_attr.attr)) {
		DMERR("Failed to create stats attribute");
		kobject_put(stats_kobj);
		return -ENOMEM;
	}

	int result = dm_register_target(&dmpt);
	if (result) {
		DMERR("Failed to register target");
		sysfs_remove_file(stats_kobj, &stats_attr.attr);
		kobject_put(stats_kobj);
	}

	return result;
}

/*
 * Unregister the target and remove sysfs on exit
 */
static void __exit proxy_exit(void) {
	sysfs_remove_file(stats_kobj, &stats_attr.attr);
	kobject_put(stats_kobj);
	dm_unregister_target(&dmpt);
}

module_init(proxy_init);
module_exit(proxy_exit);

MODULE_AUTHOR("Alexander Bugaev");
MODULE_DESCRIPTION("Proxy target collecting request statistics");
MODULE_LICENSE("GPL");

