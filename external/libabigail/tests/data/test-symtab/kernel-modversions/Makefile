obj-m += one_of_each.o

# Overwrite to an actual kernel dir when using this:
#
# $ make KDIR=/path/to/actual/kernel/source/dir
#
KDIR := /tmp/some/kernel/source/dir

PWD := $(shell pwd)
default:
	make -C $(KDIR) tinyconfig
	cd $(KDIR) && ./scripts/config -e 64BIT -e MODULES -e MODVERSIONS -e CONFIG_DEBUG_INFO
	$(MAKE) -C $(KDIR) olddefconfig
	$(MAKE) -C $(KDIR)
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	rm -rf *.mod.c *.o .*.cmd .*.d *.mod modules.order Module.symvers .tmp_versions

clean:
	rm -f *.ko
