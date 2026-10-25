#include "bwt.h"

static int default_alphabet_map(char c, void *udata) {
  // return positive integer representation of the character
  return *(unsigned char *)&c;
}

static struct alphabet default_alphabet = {
    .size = 255,
    .map = default_alphabet_map,
    .udata = NULL,
};

arr(char) bwt_compress(const char *inputs[], size_t lengths[], size_t count,
                       struct alphabet *alphabet) {
  if (!alphabet)
    alphabet = &default_alphabet;

  size_t max_length = 0;
  for (size_t i = 0; i < count; i++) {
    if (lengths[i] > max_length)
      max_length = lengths[i];
  }

  size_t sum_lengths = 0;
  for (size_t i = 0; i < count; i++) {
    sum_lengths += lengths[i];
  }

  // prepare output memory to hold all inputs and the '\0' terminators
  arr(char) result = NULL;
  arrsetcap(result, sum_lengths + count);

  int *count_smaller = calloc(alphabet->size + 1, sizeof(count_smaller[0]));

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

  for (size_t i = 0; i < count; i++) {
    order[i].position = 0;
    order[i].index = i;
  }

  // insert all characters plus the `\0` terminators
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

    // Collect ranks
    for (size_t j = 0; j < count; j++) {
      if (done[j])
        continue;

      ranks[j] = char_rank(result, order[j].character, order[j].position);
    }

    // Insert
    for (int j = count - 1; j >= 0; j--) {
      if (done[j])
        continue;

      arrins(result, order[j].position, order[j].character);
    }

    // Calculate new positions
    for (size_t j = 0; j < count; j++) {
      if (done[j])
        continue;

      int ord = alphabet->map(order[j].character, alphabet->udata);
      order[j].position = ranks[j] + count_smaller[ord] + num_chars - 1;
    }

    // Update count array
    for (size_t j = 0; j < count; j++) {
      if (done[j])
        continue;

      int ord = alphabet->map(order[j].character, alphabet->udata);
      for (int k = ord + 1; k < alphabet->size + 1; k++) {
        count_smaller[k]++;
      }
    }

    // Get next order by sorting by `position` and `character`
    timsort(order, count, sizeof(*order), cmp_charOrder_position_character);
  }

  free(order);
  free(done);
  free(ranks);
  free(count_smaller);

  return result;
}

arr(char)
    bwt_decompress(arr(char) bwt, size_t index, struct alphabet *alphabet) {
  if (!alphabet)
    alphabet = &default_alphabet;

  arr(char) result = NULL;

  int *count_smaller = calloc(alphabet->size + 1, sizeof(count_smaller[0]));

  // collect counts
  for (size_t i = 0; i < arrlenu(bwt); i++) {
    int ord = alphabet->map(bwt[i], alphabet->udata);
    if (ord + 1 < alphabet->size + 1)
      count_smaller[ord + 1]++;
  }

  // accumulate counts
  for (int i = 1; i < alphabet->size + 1; i++) {
    count_smaller[i] += count_smaller[i - 1];
  }

  while (true) {
    char c = bwt[index];
    if (c == '\0')
      break;

    arrins(result, 0, c);

    int ord = alphabet->map(c, alphabet->udata);
    index = count_smaller[ord] + char_rank(bwt, c, index) - 1;
  }

  free(count_smaller);

  return result;
}
