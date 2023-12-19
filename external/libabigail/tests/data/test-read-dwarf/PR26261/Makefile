SRCS	= PR26261-obja.c PR26261-objb.c PR26261-main.c
EXE	= PR26261-exe
OBJS	= $(SRCS:.c=.o)
CFLAGS	= -Wall -g

all: $(EXE)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

$(EXE): $(OBJS)
	gcc -fPIC $(OBJS) -o $@

cleanobjs:
	rm -rf $(OBJS)

cleanexe:
	rm $(EXE)

clean: cleanobjs
	rm -rf *~ 
