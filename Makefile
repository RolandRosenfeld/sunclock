
#	Make instructions for the Sun clock program

RELFILES = README Makefile sunclock.c sunclock.h sunclock.1

LIBRARIES = -lsuntool -lsunwindow -lpixrect -lm

sunclock: sunclock.c sunclock.h
	cc -O sunclock.c -o sunclock $(LIBRARIES)
	strip sunclock

clean:
	rm -f sunclock
	rm -f *.bak
	rm -f core cscope.out

manpage:
	nroff -man sunclock.1 | more

lint:
	lint sunclock.c $(LIBRARIES)

shar:
	shar -b -v $(RELFILES) >sunclock.shar
