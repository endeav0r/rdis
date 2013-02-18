OBJS = redis_x86.o redis_x86_graph.o

CCFLAGS=-Wall -O2 -Werror -g
INCLUDE=-iquote../ -iquote../container

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

%.o : %.c
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o
