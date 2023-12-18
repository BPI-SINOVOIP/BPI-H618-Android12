

objects = check.o script.o update_uboot.o commit.o


edit:$(objects)
	gcc -o update_uboot $(objects) -static
	rm -rf $(objects)
	@cp -v update_uboot ../../../../../tools/pack/pctools/linux/mod_update/update_uboot


clean:
	rm -rf update_uboot $(objects)

include ../common/common.mk
