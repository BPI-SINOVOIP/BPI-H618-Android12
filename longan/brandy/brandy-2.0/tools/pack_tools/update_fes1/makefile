

objects = check.o script.o update_fes1.o


edit:$(objects)
	gcc -o update_fes1 $(objects) -static
	rm -rf $(objects)
	@cp -v update_fes1 ../../../../../tools/pack/pctools/linux/mod_update/update_fes1


clean:
	rm -rf update_fes1 $(objects)
