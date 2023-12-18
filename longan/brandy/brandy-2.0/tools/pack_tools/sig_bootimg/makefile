

objects = sig_bootimg.o


edit:$(objects)
	gcc -o sigbootimg  $(objects) -static
	rm -rf $(objects)

	cp sigbootimg   	../../../../../tools/pack/pctools/linux/mod_update/sigbootimg

clean:
	rm -rf sigbootimg  $(objects)
