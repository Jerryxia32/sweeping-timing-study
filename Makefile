CC=clang-5.0
CFLAGS=-O3
LDFLAGS=

timing: gadget.s.o timing.c.o gadget.c.o
	$(CC) $(LDFLAGS) gadget.s.o gadget.c.o timing.c.o -o timing

gadget.s.o: gadget.s
	$(CC) $(CFLAGS) -c gadget.s -o gadget.s.o

timing.c.o: timing.c gadget.h
	$(CC) $(CFLAGS) -std=c11 -c timing.c -o timing.c.o

gadget.c.o: gadget.c gadget.h
	$(CC) $(CFLAGS) -std=c11 -c gadget.c -o gadget.c.o

clean:
	rm -f *.o timing
