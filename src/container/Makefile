OBJS = buffer.o function.o graph.o index.o instruction.o label.o list.o map.o queue.o \
	rdstring.o reference.o tree.o 

CCFLAGS=-Wall -O2 -g 
INCLUDE=`pkg-config --cflags gtk+-3.0` -iquote../

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o
