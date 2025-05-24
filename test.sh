# This script will build the proxy target module, install module into the kernel,
# create test block device with proxy device over it, make several read and write requests
# and get the module statistics via sysfs.

# Exit on error
set -e

# Build and install the module
build () {
	make
	insmod dm_proxy.ko
}

# Create test devices
setup_dev() {
	dmsetup create zero1 --table "0 10000 zero"
	dmsetup create dmp1 --table "0 10000 proxy /dev/mapper/zero1"
}

# Make read and write requests
make_reqs() {
	dd if=/dev/random of=/dev/mapper/dmp1 bs=512 count=8
	dd if=/dev/mapper/dmp1 of=/dev/null bs=512 count=10
	dd if=/dev/random of=/dev/mapper/dmp1 bs=512 count=1
	dd if=/dev/mapper/dmp1 of=/dev/null bs=512 count=2
}

# Display proxy statistics
print_stats() {
	cat /sys/module/dm_proxy/stat/volumes
}

# Cleanup
cleanup() {
	exitcode=$?
	dmsetup remove dmp1
	dmsetup remove zero1
	rmmod dm_proxy
	exit $exitcode
}

trap cleanup EXIT

build
setup_dev
make_reqs
print_stats

