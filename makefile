#    Flitsim : A fully adaptable X-Windows based network simulator
#    Copyright (C) 1993  Patrick Gaughan, Sudha Yalamanchili, and Peter Flur
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#CXX = gcc  -DOLD=0 -DNO_X -pg
CXX = gcc  -DOLD=0 -DNO_X -DHASH -DPLINK -DSET_PORT 
#CXX = gcc -DOLD=0 
#CFLAGS= -c -g -I/usr/usc/X11/include
CFLAGS=-c -g
 
#AFLAGS= -S -g -I/usr/usc/X11/include
AFLAGS = -S -g

# Include XFlags on Sun
#XFLAGS = -lXext
# Exclude XFlags on HP
XFLAGS = 

#LFLAGS= -lm -L/usr/local/X11R5/lib -lX11 -lXt -lXmu $(XFLAGS) -lXaw
#LFLAGS= -lm -L/usr/usc/X11/lib -lXaw $(XFLAGS) -lXmu -lXt -lX11 
LFLAGS = -lm

.SUFFIXES: .c

.c.o: data.h dat2.h
	$(CXX) $(CFLAGS) $<

.c.s: data.h dat2.h
	$(CXX) $(AFLAGS) $<

OBJ2= route2.o mapper.o net2.o ut2.o \
	 syntraf.o tracetraf.o main2.o dd.o

ASM= route2.s mapper.s net2.s ut2.s \
	 syntraf.s tracetraf.s main2.s dd.s

#rst: $(OBJ2) 
#	$(CXX) $(OBJ2) $(LFLAGS) -o rst

rsim: $(OBJ2)
	$(CXX) $(OBJ2) $(LFLAGS) -o rsim

#xwidgets:  xwidgets.o
#	$(CXX) xwidgets.o $(LFLAGS) -o xwidgets

r3: $(OBJ2)
	$(CXX) $(OBJ2) $(LFLAGS) -o r3

r2: $(OBJ2)
	$(CXX) $(OBJ2) $(LFLAGS) -o r2

r2t: $(OBJ2)
	$(CXX) $(OBJ2) $(LFLAGS) -o r2t

rt: $(OBJ2)
	$(CXX) $(OBJ2) $(LFLAGS) -o rt

r2h: $(OBJ2)
	$(CXX) $(OBJ2) $(LFLAGS) -o r2h

mapdisp: mapdisp.o
	$(CXX) mapdisp.o $(LFLAGS) -o mapdisp

getfiles:
	co $(SRC)
	
all: rsim rst r3 r2 r2dec

asm: $(ASM)

eclean:
	rm -f *~ 

clean:
	rm -f *.o rst rsim ftest r3 *~ a.out

print-code:
	vgrind -t *.c *.h | pscat | mpage -2
