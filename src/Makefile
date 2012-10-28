OBJS=object.o rdg.o rdg_node.o rdis.o serialize.o util.o wqueue.o

CCFLAGS=-Wall -O2 -g -Werror
INCLUDE=`pkg-config --cflags cairo` -Icontainer/ -Iloader/ -I./
LIB=`pkg-config --libs gtk+-3.0 cairo` -ludis86 -lm -lfontconfig -lpthread -ljansson

all : $(OBJS)
	make -C container
	make -C gui
	make -C loader
	$(CC) -o rdis *.o container/*.o gui/*.o loader/*.o $(LIB) $(CCFLAGS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	make -C container clean
	make -C gui clean
	make -C loader clean
	rm -f *.o
	rm -f rdis
