OBJS=elf32.o elf64.o x86.o x8664.o loader.o pe.o

CCFLAGS=-Wall -O2 -Werror -g
INCLUDE=-I../container -I../

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o
