CC = gcc
CFLAGS = -Wall -Werror -g -O0 -I .
CBIBFLAG = -DUSE_PTHREAD
LIBTHREAD = -pthread
DST_TEST_BIN = install/bin
DST_TEST_LIB = install/lib
TEST_DIR = tst
TSTFILES = $(DST_TEST_BIN)/01-main.o $(DST_TEST_BIN)/02-switch.o $(DST_TEST_BIN)/03-equity.o $(DST_TEST_BIN)/11-join.o $(DST_TEST_BIN)/12-join-main.o $(DST_TEST_BIN)/21-create-many.o $(DST_TEST_BIN)/22-create-many-recursive.o $(DST_TEST_BIN)/23-create-many-once.o $(DST_TEST_BIN)/31-switch-many.o $(DST_TEST_BIN)/32-switch-many-join.o $(DST_TEST_BIN)/33-switch-many-cascade.o $(DST_TEST_BIN)/51-fibonacci.o
LIBTHREAD = $(DST_TEST_LIB)/libthread.so

all:

check:

valgrind:
	valgrind --leack-check=full --show-reachable=yes --track-origin=yes

pthreads:

graphs:

install: repositories $(LIBTHREAD) $(TSTFILES)





thread.o: thread.c
	$(CC) -fPIC $(CFLAGS) $^ -c -o $@

$(LIBTHREAD): thread.o
	$(CC) -shared -o $@ $^

$(DST_TEST_BIN)/%.o: $(TEST_DIR)/%.c $(LIBTHREAD)
	$(CC) $(CFLAGS) -c $^ -o $@

repositories:
	mkdir -p install/lib install/bin



make clean:
	rm -rf *.o example_libc install/
