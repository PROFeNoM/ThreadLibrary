CC=gcc
CFLAGS=-Wall -Werror
CBIBFLAG=-DUSE_PTHREAD
LIBTHREAD=-pthread

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
	$(CC) $^ $(CBIBFLAG) $(LIBTHREAD) -o $@

example.o: example.c
	$(CC) $(CFLAGS) $^ -c -o $@

thread.o: thread.c
	$(CC) $(CFLAGS) $^ -c -o $@

example: example.o utils.o thread.o
	$(CC) $^ $(LIBTHREAD) -o $@



make clean:
	rm -f *.o example_libc
