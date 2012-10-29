OBJS=object.o rdg.o rdg_node.o rdis.o serialize.o util.o wqueue.o

CCFLAGS=-Wall -O2 -Werror -g
INCLUDE=`pkg-config --cflags cairo` -Icontainer/ -Iloader/ -I./ -Iscript/
LIB=`pkg-config --libs gtk+-3.0 cairo` -ludis86 -lm -lfontconfig -lpthread \
	-ljansson -llua

all : $(OBJS)
	make -C container
	make -C gui
	make -C loader
	make -C script
	$(CC) -o rdis *.o container/*.o gui/*.o loader/*.o script/*.o $(LIB) $(CCFLAGS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	make -C container clean
	make -C gui clean
	make -C loader clean
	rm -f *.o
	rm -f rdis
