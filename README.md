## dm_proxy

![build-status](https://github.com/bygu4/dm-proxy-target/actions/workflows/build.yml/badge.svg)
![test-status](https://github.com/bygu4/dm-proxy-target/actions/workflows/test.yml/badge.svg)

Device mapper target that allows to collect request statistics over an existing mapper device.
Such statistics include number of requests and average block size for read and write operations separately or overall.

This module was tested for Ubuntu 24.04 and Fedora 42.

### Building the module

Building the module will require to install the corresponding Linux Kernel headers.
This can be done using the following commands:
- Ubuntu: ```sudo apt-get install -y linux-headers-$(uname -r)```
- Fedora: ```sudo dnf install kernel-devel```

After that, you can run ```make``` command to create the module executable. To clean output files you can use ```make clean```.

Finally, the module can be loaded into the kernel with the following command:
```
insmod dm_proxy.ko
```

To remove module from the kernel, you can use command ```rmmod dm_proxy```.

### Using the module

A sample of module usage can be found in ```test.sh``` file, that can also be used for testing.

Creating a proxy mapper would require specifying the base device to which the requests would be redirected to.
In general it would look like:
```
dmsetup create $(proxy_mapper_name) --table "0 $(device_size) proxy $(device_path)"
```
where ```proxy_mapper_name``` is the name of the mapper to create, and ```device_size``` is size of the device at path ```device_path```.

Statistics of the module can be accessed using sysfs. The following command would print stats to the standard output:
```
cat /sys/module/dm_proxy/stat/volumes
```
Sample statistics would look like this:
```
read:
	reqs: 243
	avg_block: 4096
write:
	reqs: 3
	avg_block: 4096
total:
	reqs: 246
	avg_block: 4096
```
