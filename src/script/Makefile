OBJS=rdis_lua.o

CCFLAGS=-Wall -O2 -Werror -g
INCLUDE=-iquote../ -iquote../container -iquote../loader -iquote./

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o
