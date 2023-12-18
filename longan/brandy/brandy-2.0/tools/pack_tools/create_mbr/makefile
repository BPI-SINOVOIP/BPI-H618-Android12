

objects = crc.o script.o update_mbr.o update_gpt.o commit.o


edit:$(objects)
	gcc -o update_mbr $(objects) -static
	rm -rf $(objects)
	@cp -v update_mbr $(COPYTODIR)/update_mbr

clean:
	rm -rf update_mbr $(objects)

include ../common/common.mk
