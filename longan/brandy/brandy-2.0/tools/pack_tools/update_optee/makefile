WARNINGS +=-Werror

CFLAGS += $(WARNINGS)

objects = update_optee.o 		\
				  ../update_scp/libfdt/fdt.o	\
				  ../update_scp/libfdt/fdt_ro.o	\
				  ../update_scp/libfdt/fdt_wip.o	\
				  ../update_scp/libfdt/fdt_sw.o	\
				  ../update_scp/libfdt/fdt_rw.o	\
				  ../update_scp/libfdt/fdt_strerror.o	\
				  ../update_scp/libfdt/fdt_empty_tree.o	\
				  ../update_scp/libscript/script.o	\

edit:$(objects)
	gcc $(CFLAGS) $(INCLUDE) -o  update_optee $(objects)
	rm -rf $(objects)
	@cp -v update_optee $(COPYTODIR)/update_optee

clean:
	rm -rf update_optee $(objects)

include ../common/common.mk