CC=gcc
CFLAGS=-Wall -Werror
CBIBFLAG=-DUSE_PTHREAD

all:

make check:

make valgrind:

make pthreads:

make graĥs:

make install:

%.o: %.c
	$(CC) $(CFLAGS) $(CBIBFLAG) $^ -c -o $@

example: example.o
	gcc example.o -pthread -o example


make clean:
	rm -f *.o example
