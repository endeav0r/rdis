OBJS=rdil.o rdil_x86.o

CCFLAGS=-Wall -O2 -Werror -g
INCLUDE=-iquote../ -iquote../container

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o
