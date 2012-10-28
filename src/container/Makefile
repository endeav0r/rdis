OBJS = buffer.o graph.o index.o instruction.o label.o list.o map.o queue.o \
	rdstring.o tree.o 

CCFLAGS=-Wall -O2 -g -Werror
INCLUDE=-I../

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o