#include "common.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

int cmp_charOrder_position_character(const void *a, const void *b) {
  struct charOrder arg1 = *(const struct charOrder *)a;
  struct charOrder arg2 = *(const struct charOrder *)b;

  if (arg1.position < arg2.position)
    return -1;
  if (arg1.position > arg2.position)
    return 1;

  if (arg1.character < arg2.character)
    return -1;
  if (arg1.character > arg2.character)
    return 1;
  return 0;
}

char *slurp(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buf = malloc(size + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  fread(buf, size, 1, f);
  fclose(f);

  buf[size] = '\0';
  return buf;
}

arr(char) concatenated_arr(arr(char) arrs[], size_t arr_count) {
  arr(char) result = NULL;

  size_t total_len = 0;
  for (size_t i = 0; i < arr_count; i++) {
    total_len += arrlen(arrs[i]);
  }

  arrsetlen(result, total_len);

  size_t idx = 0;
  for (size_t i = 0; i < arr_count; i++) {
    if (!arrs[i])
      continue;

    memcpy(&result[idx], arrs[i], arrlen(arrs[i]));
    idx += arrlen(arrs[i]);
  }

  return result;
}

size_t char_rank(const char *str, char c, size_t idx) {
  size_t n = 1;

  if (!str)
    return n;

  for (size_t i = 0; i < idx; i++) {
    if (str[i] == c)
      n++;
  }
  return n;
}

size_t char_select(const char *str, char c, size_t n) {
  size_t idx = 0;
  while (n > 0) {
    if (str[idx] == c)
      n--;
  }
  return idx;
}

int acgt_idx(char c) {
  switch (c) {
  case 'A':
  case 'a':
    return 1;
  case 'C':
  case 'c':
    return 2;
  case 'G':
  case 'g':
    return 3;
  case 'T':
  case 't':
    return 4;
  case '\0':
    return 0;
  }
  return -1;
}

int cmp_int(const void *a, const void *b) {
  int arg1 = *(const int *)a;
  int arg2 = *(const int *)b;

  if (arg1 < arg2)
    return -1;
  if (arg1 > arg2)
    return 1;
  return 0;
}

void initGenericTreeBWT(struct genericTreeBWT *self, int depth,
                        struct alphabet alphabet) {
  // methods to be implemented by subclasses
  assert(self->walk_tree);

  self->depth = depth;
  self->alphabet = alphabet;
  self->buffer_count = pow(self->alphabet.size, depth);
  arrsetlen(self->buffers, self->buffer_count);
  memset(self->buffers, 0, self->buffer_count * sizeof(self->buffers[0]));
}

void freeGenericTreeBWT(struct genericTreeBWT *self) {
  for (size_t i = 0; i < arrlenu(self->buffers); i++) {
    arrfree(self->buffers[i]);
  }
  arrfree(self->buffers);
}

void genericTreeBWT_compress(struct genericTreeBWT *self, const char *inputs[],
                             size_t lengths[], size_t count) {
  size_t max_length = 0;
  for (size_t i = 0; i < count; i++) {
    if (lengths[i] > max_length)
      max_length = lengths[i];
  }

  int *count_smaller =
      calloc(self->alphabet.size + 1, sizeof(count_smaller[0]));

  /*
   * done  stores which inputs have already been fully inserted.
   *       Does not need to be accessed in correct order when using a terminator
   *       symbol (that sorts smaller or larger than any other symbol),
   *       since any input marked as done will naturally accumulate together at
   *       the edges of the array.
   * ranks temporarily stores the ranks to compute the positions for the
   *       next iteration.
   * order the characters, the position and the index of each input sorted in
   *       insertion order.
   */
  int *ranks = calloc(count, sizeof(*ranks));
  bool *done = calloc(count, sizeof(*done));
  struct charOrder *order = malloc(count * sizeof(*order));
  int **preds = malloc(count * sizeof(preds[0]));
  struct treeTmps *tmps = malloc(count * sizeof(tmps[0]));

  for (size_t i = 0; i < count; i++) {
    preds[i] = malloc(self->depth * sizeof(preds[i][0]));
    for (int j = 0; j < self->depth; j++) {
      preds[i][j] = 1;
    }
  }

  for (size_t i = 0; i < count; i++) {
    order[i].position = 0;
    order[i].index = i;
  }

  // insert all characters plus the `\0` terminat
  for (size_t i = 0; i < max_length + 1; i++) {
    // For each input string with remaining characters
    int num_chars = 0;
    for (size_t j = 0; j < count; j++) {
      size_t idx = order[j].index;
      if (i > lengths[idx]) {
        done[j] = true;
        continue;
      }

      // Collect characters to be inserted
      if (i == lengths[idx])
        order[j].character = '\0';
      else
        order[j].character = inputs[idx][lengths[idx] - 1 - i];

      num_chars++;
    }

    memset(ranks, 0, count * sizeof(ranks[0]));

    // Walk tree to obtain ranks, tmps.localPos and tmps.bufferIdx
    self->walk_tree(self, inputs, lengths, count, ranks, done, order, preds,
                    tmps);

    // Finalize rank using regular rank in leaf node
    for (size_t j = 0; j < count; j++) {
      if (done[j])
        continue;

      assert((size_t)tmps[j].bufferIdx < self->buffer_count);
      ranks[j] += char_rank(self->buffers[tmps[j].bufferIdx],
                            order[j].character, tmps[j].localPos);
    }

    // Update predecessor
    for (size_t j = 0; j < count; j++) {
      if (done[j])
        continue;

      // Shift right and prepend inserted character
      int idx = order[j].index;
      memmove(&preds[idx][1], &preds[idx][0],
              (self->depth - 1) * sizeof(preds[idx][0]));
      preds[idx][0] =
          self->alphabet.map(order[j].character, self->alphabet.udata);
    }

    // Insert characters in leaf node
    for (int j = count - 1; j >= 0; j--) {
      if (done[j])
        continue;

      arrins(self->buffers[tmps[j].bufferIdx], tmps[j].localPos,
             order[j].character);
    }

    // Calculate new positions
    for (size_t j = 0; j < count; j++) {
      if (done[j])
        continue;

      int ord = self->alphabet.map(order[j].character, self->alphabet.udata);
      order[j].position = ranks[j] + count_smaller[ord] + num_chars - 1;
    }

    // Update count array
    for (size_t j = 0; j < count; j++) {
      if (done[j])
        continue;

      int ord = self->alphabet.map(order[j].character, self->alphabet.udata);
      for (int k = ord + 1; k < self->alphabet.size + 1; k++) {
        count_smaller[k]++;
      }
    }

    // Get next order by sorting by `position` and `character`
    timsort(order, count, sizeof(*order), cmp_charOrder_position_character);
  }

  free(done);
  free(tmps);
  for (size_t i = 0; i < count; i++) {
    free(preds[i]);
  }
  free(preds);
  free(order);
  free(ranks);
  free(count_smaller);
}
