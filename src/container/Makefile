OBJS = buffer.o function.o graph.o index.o instruction.o label.o list.o map.o queue.o \
	rdstring.o tree.o 

CCFLAGS=-Wall -O2 -g -Werror
INCLUDE=-iquote../

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o
