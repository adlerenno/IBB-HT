#include "binary-tree-bwt.h"

#include <assert.h>
#include <math.h>

struct binaryTreeAlphabetNode {
  int *count;
};

/*
 * Maps a leafIndex of a 2^self->code_len -ary tree with some empty leafs to a
 * leafIndex of a alphabet.size-ary tree
 */
static int mapLeafIdx2BufferIdx(struct binaryTreeBWT *self, int leafIdx) {
  int n = pow(2, self->code_len);
  int bufferIdx = (leafIdx / n) * self->super.alphabet.size + leafIdx % n;
  return bufferIdx;
}

static void binaryTreeBWT_walk(struct genericTreeBWT *super,
                               const char *inputs[], size_t lengths[],
                               size_t count, int *ranks, bool *done,
                               struct charOrder *order, int **preds,
                               struct treeTmps *tmps) {
  struct binaryTreeBWT *self = container_of(super, struct binaryTreeBWT, super);

  // Walk the tree: insert and calculate rank
  for (size_t j = 0; j < count; j++) {
    if (done[j])
      continue;

    tmps[j].treeIdx = 0;
    tmps[j].localPos = order[j].position;
  }

  for (int d = 0; d < self->super.depth; d++) {
    for (size_t j = 0; j < count; j++) {
      if (done[j])
        continue;

      size_t idx = order[j].index;
      tmps[j].predOrd = preds[idx][d] - 1;
    }
    for (int n = 0; n < self->code_len; n++) {
      // Walk right
      for (size_t j = 0; j < count; j++) {
        if (done[j])
          continue;

        if ((tmps[j].predOrd & (0x1 << (self->code_len - 1 - n))) != 0) {
          int characterOrd = self->super.alphabet.map(
              order[j].character, self->super.alphabet.udata);
          ranks[j] += self->nodes[tmps[j].treeIdx].count[characterOrd];
          size_t total = 0;
          for (int o = 0; o < self->super.alphabet.size + 1; o++) {
            total += self->nodes[tmps[j].treeIdx].count[o];
          }
          tmps[j].localPos -= total;
          tmps[j].treeIdx = 2 * tmps[j].treeIdx + 2;
        }
      }
      // Walk left
      for (size_t j = 0; j < count; j++) {
        if (done[j])
          continue;

        if ((tmps[j].predOrd & (0x1 << (self->code_len - 1 - n))) == 0) {
          int characterOrd = self->super.alphabet.map(
              order[j].character, self->super.alphabet.udata);
          self->nodes[tmps[j].treeIdx].count[characterOrd]++;
          tmps[j].treeIdx = 2 * tmps[j].treeIdx + 1;
        }
      }
    }
  }

  // Set buffer index
  for (size_t j = 0; j < count; j++) {
    if (done[j])
      continue;

    tmps[j].bufferIdx =
        mapLeafIdx2BufferIdx(self, tmps[j].treeIdx - self->node_count);
  }
}

struct binaryTreeBWT *buildBinaryTreeBWT(int depth, struct alphabet alphabet) {
  assert(depth > 0);

  struct binaryTreeBWT *self = calloc(1, sizeof(*self));

  self->super.walk_tree = binaryTreeBWT_walk;

  initGenericTreeBWT(&self->super, depth, alphabet);

  self->code_len = ceil(log2(self->super.alphabet.size));
  self->node_count = pow(2, depth * self->code_len) - 1;
  self->nodes = calloc(self->node_count, sizeof(self->nodes[0]));
  for (size_t i = 0; i < self->node_count; i++)
    self->nodes[i].count =
        calloc(self->super.alphabet.size + 1, sizeof(self->nodes[i].count[0]));

  return self;
}

void freeBinaryTreeBWT(struct binaryTreeBWT *self) {
  for (size_t i = 0; i < self->node_count; i++)
    free(self->nodes[i].count);
  free(self->nodes);
  freeGenericTreeBWT(&self->super);
  free(self);
}
