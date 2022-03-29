CC = gcc
CFLAGS = -Wall -Werror -g -O0 -I .
CBIBFLAG = -DUSE_PTHREAD
PTHREAD = -lpthread
USEPTHREAD ?= 0
DST_TEST_BIN = install/bin
DST_TEST_LIB = install/lib
TEST_DIR = tst
TSTFILESO = $(DST_TEST_BIN)/01-main.o $(DST_TEST_BIN)/02-switch.o $(DST_TEST_BIN)/03-equity.o $(DST_TEST_BIN)/11-join.o $(DST_TEST_BIN)/12-join-main.o $(DST_TEST_BIN)/21-create-many.o $(DST_TEST_BIN)/22-create-many-recursive.o $(DST_TEST_BIN)/23-create-many-once.o $(DST_TEST_BIN)/31-switch-many.o $(DST_TEST_BIN)/32-switch-many-join.o $(DST_TEST_BIN)/33-switch-many-cascade.o $(DST_TEST_BIN)/51-fibonacci.o
TSTFILES = $(DST_TEST_BIN)/01-main $(DST_TEST_BIN)/02-switch $(DST_TEST_BIN)/03-equity $(DST_TEST_BIN)/11-join $(DST_TEST_BIN)/12-join-main $(DST_TEST_BIN)/21-create-many $(DST_TEST_BIN)/22-create-many-recursive $(DST_TEST_BIN)/23-create-many-once $(DST_TEST_BIN)/31-switch-many $(DST_TEST_BIN)/32-switch-many-join $(DST_TEST_BIN)/33-switch-many-cascade $(DST_TEST_BIN)/51-fibonacci
LIBTHREAD = $(DST_TEST_LIB)/libthread.so

all:

check:

valgrind:
	valgrind --leak-check=full --show-reachable=yes --track-origins=yes $(TSTFILES)

pthreads:
	make USEPTHREAD=1 our_pthreads

graphs:

install: repositories $(TSTFILESO) $(LIBTHREAD) $(TSTFILES) delete_o_bin



thread.o: thread.c
ifeq ($(USEPTHREAD),0)
	$(CC) -fPIC $(CFLAGS) $^ -c -o $@
endif

$(LIBTHREAD): thread.o
ifeq ($(USEPTHREAD),0)
	$(CC) -shared -o $@ $^
endif

repositories:
	mkdir -p install/lib install/bin

$(DST_TEST_BIN)/%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(DST_TEST_BIN)/%:
ifeq ($(USEPTHREAD),1)
	$(CC) $(CFLAGS) -o $@_c $@.o $(CBIBFLAG) $(PTHREAD)
else
	$(CC) $@.o $(LIBTHREAD) -o $@
endif



our_pthreads: repositories $(TSTFILESO) $(TSTFILES) delete_o_bin



delete_o_bin:
	rm -f $(DST_TEST_BIN)/*.o

#
# example.o: example.c
# 	$(CC) $(CFLAGS) $^ -c -o $@
#
# example_libc: example.o
# 	$(CC) $^ $(CBIBFLAG) $(PTHREAD) -o $@


make clean:
	rm -rf *.o install/
