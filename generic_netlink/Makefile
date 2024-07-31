ifneq ($(KERNELRELEASE),)
	obj-m :=generic_netlink.o
else
	KERNELDIR ?=/lib/modules/$(shell uname -r)/build
	PWD :=$(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
