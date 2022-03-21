CC=gcc
CFLAGS=-Wall -Werror -g -O0
CBIBFLAG=-DUSE_PTHREAD
LIBTHREAD=-pthread
DST_TEST_DIR=install/bin
TEST_DIR=tst
TSTFILES=$(DST_TEST_DIR)/01-main.o, $(DST_TEST_DIR)/02-switch.o, $(DST_TEST_DIR)/03-equity.o, $(DST_TEST_DIR)/11-join.o, $(DST_TEST_DIR)/12-join-main.o, $(DST_TEST_DIR)/21-create-many.o, $(DST_TEST_DIR)/22-create-many-recursive.o, $(DST_TEST_DIR)/23-create-many-once.o, $(DST_TEST_DIR)/31-switch-many.o, $(DST_TEST_DIR)/32-switch-many-join.o, $(DST_TEST_DIR)/33-switch-many-cascade.o, $(DST_TEST_DIR)/51-fibonacci.o
# TSTEXECS=$($(tst)/%.c, $(install/bin)/%.o, $(TSTFILES))

all:

check:

valgrind:

pthreads:

graphs:

install: libthread.so
	mkdir -p install/{bin,lib}


test: $(TSTFILES)
	for file in $^;\
	do ;\
	$$(CC) ${file} -o ${file%.*} ;\
	done ;\

$(DST_TEST_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

libthread.so: thread.o
	$(CC) -o $^ $@ -shared

# %.o: %.c
# 	$(CC) $(CFLAGS) $(CBIBFLAG) $^ -c -o $@

utils.o: utils.c
	$(CC) $(CFLAGS) $^ -c -o $@

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
