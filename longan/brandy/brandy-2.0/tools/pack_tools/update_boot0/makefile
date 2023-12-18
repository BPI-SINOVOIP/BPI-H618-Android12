

objects = check.o script.o update_boot0.o  commit.o  \
				  ../update_scp/libfdt/fdt.o	\
				  ../update_scp/libfdt/fdt_ro.o	\
				  ../update_scp/libfdt/fdt_wip.o	\
				  ../update_scp/libfdt/fdt_sw.o	\
				  ../update_scp/libfdt/fdt_rw.o	\
				  ../update_scp/libfdt/fdt_strerror.o	\
				  ../update_scp/libfdt/fdt_empty_tree.o	\

edit:$(objects)
	gcc -o update_boot0 $(objects) -static
	rm -rf $(objects)
	@cp -v update_boot0 $(COPYTODIR)/update_boot0

clean:
	rm -rf update_boot0 $(objects)

include ../common/common.mk
