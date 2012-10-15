CC=gcc
CFLAGS = -Wall -O2 -g -Werror
LIB=`pkg-config --cflags --libs gtk+-3.0` -ludis86 -lcairo -lm

_OBJS = elf64.o graph.o index.o instruction.o list.o map.o object.o queue.o rdgraph.o \
		rdstring.o tree.o util.o x8664.o
_GTK_OBJS = hexwindow.o inswindow.o

SRCDIR = src
OBJS   = $(patsubst %,$(SRCDIR)/%,$(_OBJS))
GTK_OBJS   = $(patsubst %,$(SRCDIR)/%,$(_GTK_OBJS))

all : main gui

main : $(OBJS) $(SRCDIR)/main.o
	$(CC) $(CFLAGS) -o main $(SRCDIR)/main.o $(OBJS) -ludis86 -lcairo -lm -lfontconfig

gui : $(OBJS) $(GTK_OBJS) $(SRCDIR)/gui.o
	$(CC) $(CFLAGS) -o gui $(SRCDIR)/gui.o $(OBJS) $(GTK_OBJS) $(LIB)

%.o : %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS) $(LIB)

%.o : %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LIB)

clean :
	rm -f $(SRCDIR)/*.o
	rm -f main
