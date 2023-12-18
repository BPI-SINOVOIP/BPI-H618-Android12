

objects = parser_mbr.o

edit:$(objects)
	gcc -o parser_mbr $(objects) -static
	rm -rf $(objects)
	@cp -v parser_mbr ../../../../../tools/pack-bintools/src/parser_mbr


clean:
	rm -rf parser_mbr $(objects)
