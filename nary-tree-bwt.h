#ifndef NARY_TREE_H
#define NARY_TREE_H

#include "common.h"

struct naryTreeBWT {
  struct genericTreeBWT super;
  size_t node_count;
  struct nCountArray *nodes;
};

struct naryTreeBWT *buildNaryTreeBWT(int depth, struct alphabet alphabet);
void freeNaryTreeBWT(struct naryTreeBWT *self);

#endif
