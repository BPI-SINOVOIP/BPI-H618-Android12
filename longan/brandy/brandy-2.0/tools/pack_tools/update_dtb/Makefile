objects = update_dtb.o


edit:$(objects)
	gcc -o update_dtb $(objects) -static
	rm -rf $(objects)
	cp update_dtb ../../../../../tools/pack/pctools/linux/mod_update/update_dtb


clean:
	rm -rf update_dtb $(objects)

