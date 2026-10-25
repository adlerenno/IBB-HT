#include "huffman-tree-bwt.h"

#include <assert.h>
#include <math.h>

struct htFreqNode {
  union {
    struct {
      int ord;
    } leafNode;
    struct {
      struct htFreqNode *left;
      struct htFreqNode *right;
    } internalNode;
  } as;
  int frequency;
  bool isLeaf;
};

struct htDepthNode {
  struct huffmanNode *node;
  int depth;
  bool isUsed;
};

static arr(bool)
    compute_codes(struct huffmanNode *node, arr(bool) code, arr(bool) * codes) {
  if (node->isLeaf) {
    arr(bool) codeCpy = NULL;
    arrsetlen(codeCpy, arrlenu(code));
    memcpy(codeCpy, code, arrlenu(code));
    codes[node->as.leafNode.bufferIdx] = codeCpy;
  } else {
    arrpush(code, false);
    code = compute_codes(node->as.internalNode.left, code, codes);
    code[arrlen(code) - 1] = true;
    code = compute_codes(node->as.internalNode.right, code, codes);
    arrdel(code, arrlen(code) - 1);
  }

  return code; // to not leak memory
}

static void computeDepthsHelper(struct htFreqNode *root, int depths[],
                                int depth) {
  if (root->isLeaf) {
    depths[root->as.leafNode.ord] = depth;
  } else {
    computeDepthsHelper(root->as.internalNode.left, depths, depth + 1);
    computeDepthsHelper(root->as.internalNode.right, depths, depth + 1);
  }
}

static void computeDepths(struct htFreqNode *root, int depths[]) {
  computeDepthsHelper(root, depths, 0);
}

static void huffmanTreeBWT_walk(struct genericTreeBWT *super,
                                const char *inputs[], size_t lengths[],
                                size_t count, int *ranks, bool *done,
                                struct charOrder *order, int **preds,
                                struct treeTmps *tmps) {
  struct huffmanTreeBWT *self =
      container_of(super, struct huffmanTreeBWT, super);

  // Walk the tree: insert and calculate rank
  for (size_t j = 0; j < count; j++) {
    if (done[j])
      continue;

    tmps[j].treeIdx = 0;
    tmps[j].bufferIdx = 0;
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
      struct huffmanNode *node = self->root;
      for (int k = 0; k < arrlen(self->codes[tmps[j].predOrd]); k++) {
        if (!self->codes[tmps[j].predOrd][k]) {
          // walk left
          node = node->as.internalNode.left;
        } else {
          // walk right
          ranks[j] +=
              node->as.internalNode.counts[tmps[j].treeIdx][characterOrd];
          size_t total = 0;
          for (int o = 0; o < self->super.alphabet.size + 1; o++) {
            total += node->as.internalNode.counts[tmps[j].treeIdx][o];
          }
          tmps[j].localPos -= total;
          node = node->as.internalNode.right;
        }
      }
    }

    // Update information in nodes and position
    for (size_t j = 0; j < count; j++) {
      if (done[j])
        continue;

      int characterOrd = self->super.alphabet.map(order[j].character,
                                                  self->super.alphabet.udata);
      struct huffmanNode *node = self->root;
      for (int k = 0; k < arrlen(self->codes[tmps[j].predOrd]); k++) {
        if (!self->codes[tmps[j].predOrd][k]) {
          // walk left
          node->as.internalNode.counts[tmps[j].treeIdx][characterOrd]++;
          node = node->as.internalNode.left;
        } else {
          // walk right
          node = node->as.internalNode.right;
        }
      }

      tmps[j].treeIdx =
          self->super.alphabet.size * tmps[j].treeIdx + tmps[j].predOrd + 1;
      tmps[j].bufferIdx = self->super.alphabet.size * tmps[j].bufferIdx +
                          node->as.leafNode.bufferIdx;
    }
  }
}

struct huffmanTreeBWT *buildHuffmanTreeBWT(int depth, struct alphabet alphabet,
                                           const char *inputs[],
                                           size_t lengths[], size_t count) {
  assert(depth > 0);
  assert(inputs && lengths && count > 0);

  struct huffmanTreeBWT *self = calloc(1, sizeof(*self));
  self->super.walk_tree = huffmanTreeBWT_walk;
  initGenericTreeBWT(&self->super, depth, alphabet);

  // number of count arrays. internal n-ary node count
  self->counts_count = (pow(self->super.alphabet.size, depth) - 1) /
                       (self->super.alphabet.size - 1);

  // number of nodes for a single depth. leaf nodes + internal nodes
  self->node_count =
      self->super.alphabet.size + (self->super.alphabet.size - 1);
  self->nodes = calloc(self->node_count, sizeof(self->nodes[0]));
  self->codes = calloc(self->super.alphabet.size, sizeof(self->codes[0]));

  // Build huffman tree

  // Compute frequencies
  size_t *frequencies =
      calloc(self->super.alphabet.size, sizeof(frequencies[0]));
  for (size_t i = 0; i < count; i++) {
    for (size_t j = 0; j < lengths[i]; j++) {
      int c =
          self->super.alphabet.map(inputs[i][j], self->super.alphabet.udata) -
          1;
      frequencies[c]++;
    }
  }

  // Unbound nodes during the construction of the huffman tree
  struct htFreqNode *cNodeMemory =
      malloc(self->node_count * sizeof(*cNodeMemory));
  size_t cNodeIdx = 0;

  arr(struct htFreqNode *) unbound = NULL;

  // Add leaf nodes
  for (int i = 0; i < self->super.alphabet.size; i++) {
    struct htFreqNode *node = &cNodeMemory[cNodeIdx++];
    node->isLeaf = true;
    node->frequency = frequencies[i];
    node->as.leafNode.ord = i;
    arrpush(unbound, node);
  }
  free(frequencies);

  while (arrlen(unbound) >= 2) {
    // Find tenative-connecting pair with least frequencies
    bool hasLeast = false;
    int leastIdx1 = 0;
    int leastIdx2 = 0;
    int leastSum = 0;
    for (int idx1 = 0; idx1 < arrlen(unbound) - 1; idx1++) {
      for (int idx2 = idx1 + 1; idx2 < arrlen(unbound); idx2++) {
        bool isNextTenativeConnecting = !unbound[idx2]->isLeaf;

        int frequencySum = unbound[idx1]->frequency + unbound[idx2]->frequency;
        if (!hasLeast || frequencySum < leastSum) {
          leastIdx1 = idx1;
          leastIdx2 = idx2;
          leastSum = frequencySum;
          hasLeast = true;
        }

        if (!isNextTenativeConnecting)
          break;
      }
    }

    assert(hasLeast);

    // Merge nodes
    struct htFreqNode *parent = &cNodeMemory[cNodeIdx++];
    parent->isLeaf = false;
    parent->frequency = leastSum;
    parent->as.internalNode.left = unbound[leastIdx1];
    parent->as.internalNode.right = unbound[leastIdx2];

    // Replace left child with parent
    unbound[leastIdx1] = parent;

    // Remove right child
    arrdel(unbound, leastIdx2);
  }

  // Compute depths
  assert(arrlen(unbound) == 1); // Root is the only remaining node
  int *symbolDepths =
      malloc(self->super.alphabet.size * sizeof(symbolDepths[0]));
  computeDepths(unbound[0], symbolDepths);

  arrfree(unbound);
  free(cNodeMemory);

  // Construct alphabetical tree using computed depths
  size_t nodeIdx = 0; // we "allocate" from `nodes` by incrementing the index
  arr(struct {
    struct huffmanNode *node;
    int depth;
    bool isUsed;
  }) depths = NULL;
  arrsetlen(depths, (size_t)self->super.alphabet.size);

  for (int i = 0; i < self->super.alphabet.size; i++) {
    depths[i].depth = symbolDepths[i];
    depths[i].isUsed = true;
    depths[i].node = &self->nodes[nodeIdx++];
    depths[i].node->isLeaf = true;
    depths[i].node->as.leafNode.bufferIdx = i;
  }

  int maxDepth = 0;
  for (int i = 0; i < self->super.alphabet.size; i++)
    maxDepth = (symbolDepths[i] > maxDepth) ? symbolDepths[i] : maxDepth;
  free(symbolDepths);

  // Merge nodes according to their depth from left to right
  for (int d = maxDepth; d > 0; d--) {
    for (size_t i = 0; i < arrlenu(depths); i++) {
      if (!depths[i].isUsed || depths[i].depth != d)
        continue;

      // Find adjacent node with same depth
      size_t otherIdx = 0;
      for (size_t j = i + 1; j < arrlenu(depths); j++) {
        if (depths[j].isUsed && depths[j].depth == d) {
          otherIdx = j;
          break;
        }
      }
      assert(otherIdx > 0);

      // Merge nodes:
      // replace left node with a new parent
      struct huffmanNode *left = depths[i].node;
      depths[i].node = &self->nodes[nodeIdx++];
      depths[i].node->isLeaf = false;
      depths[i].node->as.internalNode.counts =
          calloc(self->counts_count,
                 sizeof(depths[i].node->as.internalNode.counts[0]));
      for (size_t c = 0; c < self->counts_count; c++) {
        depths[i].node->as.internalNode.counts[c] =
            calloc(self->super.alphabet.size + 1,
                   sizeof(depths[i].node->as.internalNode.counts[c][0]));
      }
      depths[i].node->as.internalNode.left = left;
      depths[i].node->as.internalNode.right = depths[otherIdx].node;
      depths[i].depth--;

      // remove right node
      depths[otherIdx].isUsed = false;

      i = otherIdx;
    }
  }

  // Root should now be the left-most node
  assert(depths[0].isUsed);
  self->root = depths[0].node;
  arrfree(depths);

  // Compute codes by walking the tree
  arr(bool) code = NULL;
  code = compute_codes(self->root, code, self->codes);
  arrfree(code);

  return self;
}

void freeHuffmanTreeBWT(struct huffmanTreeBWT *self) {
  for (int i = 0; i < self->super.alphabet.size; i++)
    arrfree(self->codes[i]);
  free(self->codes);
  for (size_t i = 0; i < self->node_count; i++) {
    if (!self->nodes[i].isLeaf) {
      for (size_t c = 0; c < self->counts_count; c++) {
        free(self->nodes[i].as.internalNode.counts[c]);
      }
      free(self->nodes[i].as.internalNode.counts);
    }
  }
  free(self->nodes);
  freeGenericTreeBWT(&self->super);
  free(self);
}
