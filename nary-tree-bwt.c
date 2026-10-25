#include "nary-tree-bwt.h"

#include <assert.h>
#include <math.h>

struct nCountArray {
  int **counts; // `alphabet.size - 1` branches (except right-most one) with
                // `alphabet.size + 1` symbols
};

static void naryTreeBWT_walk(struct genericTreeBWT *super, const char *inputs[],
                             size_t lengths[], size_t count, int *ranks,
                             bool *done, struct charOrder *order, int **preds,
                             struct treeTmps *tmps) {
  struct naryTreeBWT *self = container_of(super, struct naryTreeBWT, super);

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

    // Adjust rank and localPosition using information in nodes
    for (size_t j = 0; j < count; j++) {
      if (done[j])
        continue;

      int characterOrd = self->super.alphabet.map(order[j].character,
                                                  self->super.alphabet.udata);
      for (int p = 0; p < tmps[j].predOrd; p++) {
        ranks[j] += self->nodes[tmps[j].treeIdx].counts[p][characterOrd];
      }

      size_t total = 0;
      for (int p = 0; p < tmps[j].predOrd; p++) {
        for (int o = 0; o < super->alphabet.size + 1; o++) {
          total += self->nodes[tmps[j].treeIdx].counts[p][o];
        }
      }
      tmps[j].localPos -= total;
    }

    // Update information in nodes and position
    for (size_t j = 0; j < count; j++) {
      if (done[j])
        continue;

      int characterOrd = self->super.alphabet.map(order[j].character,
                                                  self->super.alphabet.udata);
      if (tmps[j].predOrd < self->super.alphabet.size - 1)
        self->nodes[tmps[j].treeIdx].counts[tmps[j].predOrd][characterOrd]++;

      tmps[j].treeIdx =
          super->alphabet.size * tmps[j].treeIdx + tmps[j].predOrd + 1;
    }
  }

  // Set buffer index
  for (size_t j = 0; j < count; j++) {
    if (done[j])
      continue;

    tmps[j].bufferIdx = tmps[j].treeIdx - self->node_count;
  }
}

struct naryTreeBWT *buildNaryTreeBWT(int depth, struct alphabet alphabet) {
  assert(depth > 0);
  struct naryTreeBWT *self = calloc(1, sizeof(*self));
  self->super.walk_tree = naryTreeBWT_walk;
  initGenericTreeBWT(&self->super, depth, alphabet);

  self->node_count = (pow(self->super.alphabet.size, depth) - 1) /
                     (self->super.alphabet.size - 1);
  self->nodes = calloc(self->node_count, sizeof(self->nodes[0]));

  // Allocate count arrays
  for (size_t i = 0; i < self->node_count; i++) {
    self->nodes[i].counts =
        calloc(self->super.alphabet.size - 1, sizeof(self->nodes[i].counts[0]));
    for (int j = 0; j < self->super.alphabet.size - 1; j++)
      self->nodes[i].counts[j] = calloc(self->super.alphabet.size + 1,
                                        sizeof(self->nodes[i].counts[j][0]));
  }

  return self;
}

void freeNaryTreeBWT(struct naryTreeBWT *self) {
  for (size_t i = 0; i < self->node_count; i++) {
    for (int j = 0; j < self->super.alphabet.size - 1; j++)
      free(self->nodes[i].counts[j]);
    free(self->nodes[i].counts);
  }
  free(self->nodes);
  freeGenericTreeBWT(&self->super);
  free(self);
}
