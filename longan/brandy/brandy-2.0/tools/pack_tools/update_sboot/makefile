

objects = check.o script.o update_sboot.o   commit.o  \
				  ../update_scp/libfdt/fdt.o	\
				  ../update_scp/libfdt/fdt_ro.o	\
				  ../update_scp/libfdt/fdt_wip.o	\
				  ../update_scp/libfdt/fdt_sw.o	\
				  ../update_scp/libfdt/fdt_rw.o	\
				  ../update_scp/libfdt/fdt_strerror.o	\
				  ../update_scp/libfdt/fdt_empty_tree.o	\


edit:$(objects)
	gcc -o update_sboot $(objects) -static
	rm -rf $(objects)
	@cp -v update_sboot ../../../../../tools/pack/pctools/linux/mod_update/update_sboot

clean:
	rm -rf update_sboot $(objects)

include ../common/common.mk
