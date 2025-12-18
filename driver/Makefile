obj-m += pankha.o

KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	@if lsmod | grep -q 'pankha'; then sudo modprobe -r pankha; fi
	sudo cp pankha.ko /lib/modules/$(shell uname -r)/kernel/drivers
	sudo depmod
	sudo modprobe pankha
	sudo sh -c 'echo "pankha" >> /etc/modules-load.d/pankha.conf'

