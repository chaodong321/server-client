obj-m += my_genl_module.o

CFLAGS := -Wall -O2

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o my_genl_user my_genl_user.c -I/usr/include/libnl3 -lnl-genl-3 -lnl-3

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f my_genl_user