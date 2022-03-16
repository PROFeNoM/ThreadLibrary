CC=gcc
CFLAGS=-Wall -Werror
CBIBFLAG=-DUSE_PTHREAD

all:

make check:

make valgrind:

make pthreads:

make graphs:

make install:

# %.o: %.c
# 	$(CC) $(CFLAGS) $(CBIBFLAG) $^ -c -o $@

utils.o: utils.c
	$(CC) $^ -c -o $@

example_libc: example.o
	$(CC) $^ -pthread -o $@

example.o: example.c
	$(CC) $(CFLAGS) $^ -c -o $@

example: example.o utils.o
	$(CC) $^ -pthread -o $@


make clean:
	rm -f *.o example_libc
