.POSIX:
CFLAGS = -Wall -Wextra -Wpedantic \
	-Wno-unused-function -Wno-unused-result -Wno-unused-parameter \
	-Wno-implicit-fallthrough \
	-std=c99 \
	-O2
LDFLAGS = -lm 
OBJS = \
	bwt.o \
	binary-tree-bwt.o \
	huffman-tree-bwt.o \
	nary-tree-bwt.o \
	common.o

.PHONY: all
all: test-bwt benchmark

# different LDFLAGS order (after input `$<`) for correct linkage
test-bwt: test-bwt.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# different LDFLAGS order (after input `$<`) for correct linkage
benchmark: benchmark.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: test
test: test-bwt
	./test-bwt

.PHONY: clean
clean:
	rm -f main test-bwt $(OBJS)
