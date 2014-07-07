OBJS=rdis_lua.o rl_redis_x86.o

CCFLAGS=-Wall -O2 -g
INCLUDE=`pkg-config --cflags cairo gtk+-3.0` \
	-iquote../ -iquote../container -iquote../loader -iquote./ -iquote../redis -iquote../gui

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o
