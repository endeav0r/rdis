OBJS = hexwindow.o inswindow.o funcwindow.o main.o refwindow.o rdgwindow.o \
		rdiswindow.o tipwindow.o gui.o

CCFLAGS=-Wall -O2 -Werror -g
INCLUDE=`pkg-config --cflags gtk+-3.0` \
        -iquote../ -iquote../container -iquote../loader -iquote../script

all : $(OBJS)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

%.o : %.c
	$(CC) -c -o $@ $< $(INCLUDE) $(CCFLAGS)

clean :
	rm -f *.o
