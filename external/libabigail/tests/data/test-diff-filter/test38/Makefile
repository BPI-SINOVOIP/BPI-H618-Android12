SRCS = test38-a.c test38-b.c test38-c.c
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -g

all: test38-v0 test38-v1

.PHONY: clean cleanobjs

%.o: %.c
	$(CC) $(CFLAGS) -c $<

test38-v0: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test38-v1: $(OBJS)
	$(CC) $(CFLAGS) -o $@ test38-a.o test38-c.o test38-b.o

cleanobjs:
	rm -rf $(OBJS)

clean: cleanobjs
	rm -rf test38-v0 test38-v1 *~
