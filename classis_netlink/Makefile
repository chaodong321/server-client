ifneq ($(KERNELRELEASE),)
	obj-m :=classis_netlink.o
else
	KERNELDIR ?=/lib/modules/$(shell uname -r)/build
	PWD :=$(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
gcc -o classis_netlink_user classis_netlink_user.c 
