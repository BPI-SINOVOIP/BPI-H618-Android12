srcs = $(wildcard *.c)
libs = $(srcs:.c=.so)

flags = -fPIC

all:	$(libs)

clean:
	rm $(libs)

no_debug_info.so: no_debug_info.c Makefile
	$(CC) $< -shared $(flags) -o $@

link_against_me.so: link_against_me.c
	$(CC) $< -shared $(flags) -o $@

%.so: %.c Makefile link_against_me.so
	$(CC) $< -shared $(flags) -g -o $@ link_against_me.so

