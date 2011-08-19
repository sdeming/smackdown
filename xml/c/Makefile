CFLAGS=-Wall -O2 -funroll-all-loops -funswitch-loops -ffast-math -finline-functions

xml: xml.o
	gcc $(CFLAGS) -o xml xml.o

xml.o: xml.c
	gcc $(CFLAGS) -o xml.o -c xml.c


clean:
	rm -f xml xml.o
