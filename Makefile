CC = gcc
CFLAGS = -Wall -Werror -g -O0 -I .
CBIBFLAG = -DUSE_PTHREAD
PTHREAD = -lpthread
USEPTHREAD ?= 0
DST_TEST_BIN = install/bin
DST_TEST_LIB = install/lib
TEST_DIR = tst
TSTFILESO = $(TSTFILES:%=%.o)
TSTFILES = 	$(DST_TEST_BIN)/01-main \
						$(DST_TEST_BIN)/02-switch \
						$(DST_TEST_BIN)/03-equity \
						$(DST_TEST_BIN)/11-join \
						$(DST_TEST_BIN)/12-join-main \
						$(DST_TEST_BIN)/21-create-many \
						$(DST_TEST_BIN)/22-create-many-recursive \
						$(DST_TEST_BIN)/23-create-many-once \
						$(DST_TEST_BIN)/31-switch-many \
						$(DST_TEST_BIN)/32-switch-many-join \
						$(DST_TEST_BIN)/33-switch-many-cascade \
						$(DST_TEST_BIN)/51-fibonacci \
						$(DST_TEST_BIN)/61-mutex \
						$(DST_TEST_BIN)/62-mutex \
						# $(DST_TEST_BIN)/81-deadlock
LIBTHREAD = $(DST_TEST_LIB)/libthread.so
THREADONLY =	$(DST_TEST_BIN)/21-create-many \
							$(DST_TEST_BIN)/22-create-many-recursive \
							$(DST_TEST_BIN)/23-create-many-once
							# $(DST_TEST_BIN)/61-mutex
							# $(DST_TEST_BIN)/62-mutex
THREADANDYIELD =	$(DST_TEST_BIN)/31-switch-many \
									$(DST_TEST_BIN)/32-switch-many-join \
									$(DST_TEST_BIN)/33-switch-many-cascade
LDLIBRARYPATH = ./install/lib/
LIBTHREADNAME = thread
TSTFILESWITHOUTARGS = $(DST_TEST_BIN)/01-main \
											$(DST_TEST_BIN)/02-switch \
											$(DST_TEST_BIN)/03-equity \
											$(DST_TEST_BIN)/11-join \
											$(DST_TEST_BIN)/12-join-main
TSTFILESWITHARGS1 =	$(DST_TEST_BIN)/21-create-many \
										$(DST_TEST_BIN)/22-create-many-recursive \
										$(DST_TEST_BIN)/23-create-many-once \
										$(DST_TEST_BIN)/61-mutex \
										$(DST_TEST_BIN)/62-mutex
TSTFILESWITHARGS2 =	$(DST_TEST_BIN)/31-switch-many \
										$(DST_TEST_BIN)/32-switch-many-join
TSTFILESWITHARGS3 =	$(DST_TEST_BIN)/33-switch-many-cascade
TSTFILESWITHARGS4 =	$(DST_TEST_BIN)/51-fibonacci \


all: install

check: install exec

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
	$(CC) $@.o -L$(LDLIBRARYPATH) -o $@ -l$(LIBTHREADNAME)
endif



our_pthreads: repositories $(TSTFILESO) $(TSTFILES) delete_o_bin



repo_graph:
	mkdir -p install/graphs

save_graphs:
	for file_1 in $(THREADONLY) ; do \
		LD_LIBRARY_PATH=$(LDLIBRARYPATH) taskset -c 0 python3 perf.py $$file_1 1000000 ; \
	done
	for file_2 in $(THREADANDYIELD) ; do \
		LD_LIBRARY_PATH=$(LDLIBRARYPATH) taskset -c 0 python3 perf.py $$file_2 1000000 10 ; \
	done

delete_o_bin:
	rm -f $(DST_TEST_BIN)/*.o


exec:
	for file_1 in $(TSTFILESWITHOUTARGS) ; do \
		LD_LIBRARY_PATH=$(LDLIBRARYPATH) ./$$file_1 ; \
	done
	for file_2 in $(TSTFILESWITHARGS1) ; do \
		LD_LIBRARY_PATH=$(LDLIBRARYPATH) ./$$file_2 20 ; \
	done
	for file_3 in $(TSTFILESWITHARGS2) ; do \
		LD_LIBRARY_PATH=$(LDLIBRARYPATH) ./$$file_3 10 20 ; \
	done
	for file_4 in $(TSTFILESWITHARGS3) ; do \
		LD_LIBRARY_PATH=$(LDLIBRARYPATH) ./$$file_4 20 5 ; \
	done
	for file_5 in $(TSTFILESWITHARGS4) ; do \
		LD_LIBRARY_PATH=$(LDLIBRARYPATH) ./$$file_5 8 ; \
	done



make clean:
	rm -rf *.o install/
