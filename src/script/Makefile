OBJS=rdis_lua.o

CCFLAGS=-Wall -O2 -Werror -g
INCLUDE=`pkg-config --cflags cairo` \
	-iquote../ -iquote../container -iquote../loader -iquote./ -iquote../redis

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o
