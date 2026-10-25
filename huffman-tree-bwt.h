#ifndef HUFFMAN_TREE_BWT_H
#define HUFFMAN_TREE_BWT_H

#include "common.h"

struct huffmanNode {
  union {
    struct {
      int **counts; // `counts_count` counts arrays with `alphabet.size + 1`
                    // symbols
      struct huffmanNode *left;
      struct huffmanNode *right;
    } internalNode;
    struct {
      int bufferIdx;
    } leafNode;
  } as;
  bool isLeaf;
};

struct huffmanTreeBWT {
  struct genericTreeBWT super;
  size_t node_count;
  size_t counts_count;
  struct huffmanNode *nodes;
  struct huffmanNode *root;
  arr(bool) * codes;
};

struct huffmanTreeBWT *buildHuffmanTreeBWT(int depth, struct alphabet alphabet,
                                           const char *inputs[],
                                           size_t lengths[], size_t count);
void freeHuffmanTreeBWT(struct huffmanTreeBWT *self);

#endif
