OBJS = hexwindow.o inswindow.o funcwindow.o main.o rdgwindow.o rdiswindow.o gui.o

CCFLAGS=-Wall -O2 -Werror -g
INCLUDE=-I../ -I../container -I../loader -I../script `pkg-config --cflags gtk+-3.0`

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

%.o : %.c
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o
