WARNINGS +=-Werror

CFLAGS += $(WARNINGS)

objects = sunxi_ubootools.o commit.o	\
				  ../update_scp/libfdt/fdt.o	\
				  ../update_scp/libfdt/fdt_ro.o	\
				  ../update_scp/libfdt/fdt_wip.o	\
				  ../update_scp/libfdt/fdt_sw.o	\
				  ../update_scp/libfdt/fdt_rw.o	\
				  ../update_scp/libfdt/fdt_strerror.o	\
				  ../update_scp/libfdt/fdt_empty_tree.o	\
				  ../update_scp/libscript/script.o	\
				  ../update_uboot/check.o	\

.PHONY:edit
edit:$(objects)
	gcc $(INCLUDE) $(CFLAGS) -o sunxi_ubootools $(objects)
	rm -rf $(objects)
	@cp -v sunxi_ubootools $(COPYTODIR)/sunxi_ubootools
	@cp -v sunxi_ubootools ../../../../brandy-2.0/u-boot-2018/scripts/sunxi_ubootools

clean:
	rm -rf sunxi_ubootools $(objects)

include ../common/common.mk