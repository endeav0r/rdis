OBJS=rdis_lua.o

CCFLAGS=-Wall -O2 -Werror -g
INCLUDE=-I../ -I../container -I../loader -I./

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o
