

objects = script.o


edit:$(objects)
	gcc -o script $(objects) -static
	rm -rf $(objects)
	cp script ../../../../../tools/pack/pctools/linux/mod_update/script


clean:
	rm -rf script $(objects)

