OBJS = hexwindow.o inswindow.o funcwindow.o rdgwindow.o rdiswindow.o gui.o

CCFLAGS=-Wall -O2 -g -Werror
INCLUDE=-I../ -I../container -I../loader `pkg-config --cflags gtk+-3.0`

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o
