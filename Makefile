CC = gcc
CFLAGS = -Wall -Werror -g -O0 -I .
CBIBFLAG = -DUSE_PTHREAD
PTHREAD = -lpthread
USEPTHREAD ?= 0
DST_TEST_BIN = install/bin
DST_TEST_LIB = install/lib
TEST_DIR = tst
TSTFILESO = $(TSTFILES:%=%.o)
TSTFILES = $(DST_TEST_BIN)/01-main $(DST_TEST_BIN)/02-switch $(DST_TEST_BIN)/03-equity $(DST_TEST_BIN)/11-join $(DST_TEST_BIN)/12-join-main $(DST_TEST_BIN)/21-create-many $(DST_TEST_BIN)/22-create-many-recursive $(DST_TEST_BIN)/23-create-many-once $(DST_TEST_BIN)/31-switch-many $(DST_TEST_BIN)/32-switch-many-join $(DST_TEST_BIN)/33-switch-many-cascade $(DST_TEST_BIN)/51-fibonacci $(DST_TEST_BIN)/61-mutex $(DST_TEST_BIN)/62-mutex# $(DST_TEST_BIN)/81-deadlock
LIBTHREAD = $(DST_TEST_LIB)/libthread.so
THREADONLY = $(DST_TEST_BIN)/21-create-many $(DST_TEST_BIN)/22-create-many-recursive $(DST_TEST_BIN)/23-create-many-once $(DST_TEST_BIN)/61-mutex $(DST_TEST_BIN)/62-mutex
THREADANDYIELD = $(DST_TEST_BIN)/31-switch-many $(DST_TEST_BIN)/32-switch-many-join $(DST_TEST_BIN)/33-switch-many-cascade

all:
	make install

check:

valgrind:
	valgrind --leak-check=full --show-reachable=yes --track-origins=yes $(TSTFILES)

pthreads:
	make -B USEPTHREAD=1 our_pthreads

graphs: repo_graph save_graphs

install: repositories $(TSTFILESO) $(LIBTHREAD) $(TSTFILES) delete_o_bin


utils.o: utils.c
ifeq ($(USEPTHREAD),0)
	$(CC) -fPIC $(CFLAGS) $^ -c -o $@
endif
thread.o: thread.c
ifeq ($(USEPTHREAD),0)
	$(CC) -fPIC $(CFLAGS) $^ -c -o $@
endif

$(LIBTHREAD): thread.o utils.o
ifeq ($(USEPTHREAD),0)
	$(CC) -shared -o $@ $^
endif

repositories:
	mkdir -p install/lib install/bin

$(DST_TEST_BIN)/%.o: $(TEST_DIR)/%.c
ifeq ($(USEPTHREAD),1)
	$(CC) $(CFLAGS) -c $^ -o $@ $(CBIBFLAG)
else
	$(CC) $(CFLAGS) -c $^ -o $@
endif

$(DST_TEST_BIN)/%:
ifeq ($(USEPTHREAD),1)
	$(CC) -o $@_c $@.o $(PTHREAD)
else
	$(CC) $@.o $(LIBTHREAD) -o $@
endif



our_pthreads: repositories $(TSTFILESO) $(TSTFILES) delete_o_bin



repo_graph:
	mkdir -p install/graphs

save_graphs:
	for file_1 in $(THREADONLY) ; do \
		python3 perf.py $$file_1 100 ; \
	done
	for file_2 in $(THREADANDYIELD) ; do \
		python3 perf.py $$file_2 100 5 ; \
	done

delete_o_bin:
	rm -f $(DST_TEST_BIN)/*.o


make clean:
	rm -rf *.o install/
