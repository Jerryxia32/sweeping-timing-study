CC=clang50
CFLAGS=-O3
LDFLAGS=

timing: gadget.o timing.o
	$(CC) $(LDFLAGS) gadget.o timing.o -o timing

gadget.o: gadget.s
	$(CC) $(CFLAGS) -c gadget.s -o gadget.o

timing.o: timing.c
	$(CC) $(CFLAGS) -std=c11 -c timing.c -o timing.o

clean:
	rm -f *.o timing
