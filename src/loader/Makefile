OBJS=elf32.o elf64.o x86.o x8664.o loader.o

CCFLAGS=-Wall -O2 -g -Werror
INCLUDE=-I../container -I../

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o
