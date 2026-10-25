#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>

#include "deps/timsort/timsort.h"

/* data structures (dynamic arrays and hash tables) */
#include "deps/stb_ds.h"

/* define a macro to make explicit that a pointer is allocated by stb_ds.h */
#define arr(X) X *

/* a macro to get the length of static arrays */
#define ARRLEN(X) ((size_t)(sizeof(X) / sizeof(*(X))))

/* gets a pointer to the containing struct from a pointer to a member */
#define container_of(ptr, type, member)                                        \
  ((type *)((char *)(ptr)-offsetof(type, member)))

struct charOrder {
  size_t index;
  size_t position;
  char character;
};

struct treeTmps {
  int localPos;
  int predOrd;
  int treeIdx;
  int bufferIdx;
};

/**
 * Alphabet type represents an alphabet that fits into a char.
 *
 * size  is the number of symbols in the alphabet
 * map   defines a function to map symbols encoded in a char to indices
 *       (terminator '\0' should be mapped to 0 and any other symbol 1 and up)
 * udata may be used as a context pointer for the `map` function
 *       (e.g. to model closures)
 */
struct alphabet {
  int size;
  int (*map)(char c, void *udata);
  void *udata;
};

struct genericTreeBWT {
  int depth;
  struct alphabet alphabet;
  size_t buffer_count;
  arr(arr(char)) buffers;

  void (*walk_tree)(struct genericTreeBWT *super, const char *inputs[],
                    size_t lengths[], size_t count, int *ranks, bool *done,
                    struct charOrder *order, int **preds,
                    struct treeTmps *tmps);
};

int cmp_charOrder_position_character(const void *a, const void *b);

/// Reads all data from a file.
char *slurp(const char *filename);

arr(char) concatenated_arr(arr(char) arrs[], size_t arr_count);

size_t char_rank(const char *str, char c, size_t idx);

size_t char_select(const char *str, char c, size_t n);

int acgt_idx(char c);

int cmp_int(const void *a, const void *b);

void initGenericTreeBWT(struct genericTreeBWT *self, int depth,
                        struct alphabet alphabet);

void freeGenericTreeBWT(struct genericTreeBWT *self);

void genericTreeBWT_compress(struct genericTreeBWT *self, const char *inputs[],
                             size_t lengths[], size_t count);

#endif
