
MAIN_SRC = test39-main.c

V0_SRCS = \
test39-a-v0.c \
test39-b-v0.c \
test39-c-v0.c

V1_SRCS = \
test39-a-v1.c \
test39-b-v1.c \
test39-c-v1.c

SRCS = $(V0_SRCS) $(V1_SRCS)

V0_OBJS = $(V0_SRCS:.c=.o)
V1_OBJS = $(V1_SRCS:.c=.o)

OBJS = $(V0_OBJS) $(V1_OBJS)

CFLAGS = -g -Wall

all: test39-v0 test39-v1
.PHONY: cleanobjs clean

%.o: %.c
	$(CC) $(CFLAGS) -c $<

test39-v0: $(V0_OBJS)
	$(CC) $(CFLAGS) -o $@ $(MAIN_SRC) $(V0_OBJS)

test39-v1: $(V1_OBJS)
	$(CC) $(CFLAGS) -o $@ $(MAIN_SRC) $(V1_OBJS)

cleanobjs:
	rm -rf $(OBJS) *~

clean: cleanobjs
	rm -rf test39-v0 test39-v1

