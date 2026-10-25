#ifndef BINARY_TREE_BWT_H
#define BINARY_TREE_BWT_H

#include "common.h"

struct binaryTreeBWT {
  struct genericTreeBWT super;
  size_t node_count;
  struct binaryTreeAlphabetNode *nodes;
  int code_len;
};

struct binaryTreeBWT *buildBinaryTreeBWT(int depth, struct alphabet alphabet);
void freeBinaryTreeBWT(struct binaryTreeBWT *self);

#endif
