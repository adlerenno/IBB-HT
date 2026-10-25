#include "common.h"

#include "binary-tree-bwt.h"
#include "bwt.h"
#include "huffman-tree-bwt.h"
#include "nary-tree-bwt.h"
#include <stdio.h>
#include <time.h>

/* include implementations for easier compilation */
#define STB_DS_IMPLEMENTATION
#include "deps/stb_ds.h"
#include "deps/timsort/timsort.c"

/* Benchmark parameters */
#define RUN_COUNT 10
static struct {
  int alphabet_size;
  int max_depth;
} params[] = {
    {2, 10}, {4, 10}, {8, 9}, {16, 6}, {32, 5}, {64, 4}, {128, 3}, {255, 3},
};

struct benchmark_results {
  clock_t start;
  clock_t prepare;
  clock_t conclude;
  clock_t end;
};

static size_t input_count = 0;
static char **inputs = NULL;
static char **reduced_inputs = NULL;
static size_t *lengths = NULL;
static int tree_depth = 1;

void do_nothing(void *ptr) { (void)ptr; }

void print_results(const char *name, const int alphabet_size, const int depth, struct benchmark_results runs[],
                   size_t run_count) {
  printf("Benchmark (%zu runs): %s\n", run_count, name);
  FILE *csv_file = fopen("benchmarks.csv", "a");
  if (csv_file == NULL) {
    perror("Error opening CSV file");
    return;
  }
  fseek(csv_file, 0, SEEK_END);
  if (ftell(csv_file) == 0) {
    fprintf(csv_file, "benchmark_name,part,alphabet_size,depth,mean_s,min_s,max_s,runs\n");
  }

  for (int part = 0; part < 3; part++) {
    double mean = 0.0;
    double min = 0;
    double max = 0;
    for (size_t i = 0; i < run_count; i++) {
      double t;
      if (part == 0)
        t = (double)(runs[i].prepare - runs[i].start) / CLOCKS_PER_SEC;
      else if (part == 1)
        t = (double)(runs[i].conclude - runs[i].prepare) / CLOCKS_PER_SEC;
      else
        t = (double)(runs[i].end - runs[i].conclude) / CLOCKS_PER_SEC;

      mean += t;

      if (i == 0) {
        min = t;
        max = t;
      } else {
        if (t < min)
          min = t;
        if (t > max)
          max = t;
      }
    }
    mean /= run_count;

    if (part == 0) {
      printf("  prepare: ");
      fprintf(csv_file, "%s,%s,%d,%d,%.6f,%.6f,%.6f,%zu\n",
                name, "prepare", alphabet_size, depth, mean, min, max, run_count);
    }
    else if (part == 1) {
      printf("  run:     ");
      fprintf(csv_file, "%s,%s,%d,%d,%.6f,%.6f,%.6f,%zu\n",
                name, "run", alphabet_size, depth, mean, min, max, run_count);
    }
    else {
      printf("  conclude:");
      fprintf(csv_file, "%s,%s,%d,%d,%.6f,%.6f,%.6f,%zu\n",
                name, "conclude", alphabet_size, depth, mean, min, max, run_count);
    }

    printf(" mean:      %.3f s\n", mean);
    printf("            min … max: %.3f s … %.3f s\n", min, max);
  }
  fclose(csv_file);
}

static int reduced_map(char c, void *udata) {
  // assume input has already been reduced
  return *(unsigned char *)&c;
}

static struct alphabet reduced_alphabet = {
    .size = 255,
    .map = reduced_map,
    .udata = NULL,
};

void bench_simple(struct benchmark_results *run) {
  run->start = clock();
  run->prepare = clock();

  arr(char) bwt = bwt_compress((const char **)reduced_inputs, lengths,
                               input_count, &reduced_alphabet);

  run->conclude = clock();

  do_nothing(&bwt);
  arrfree(bwt);

  run->end = clock();
}

void bench_binary(struct benchmark_results *run) {
  run->start = clock();

  struct binaryTreeBWT *tree = buildBinaryTreeBWT(tree_depth, reduced_alphabet);

  run->prepare = clock();

  genericTreeBWT_compress(&tree->super, (const char **)reduced_inputs, lengths,
                          input_count);

  run->conclude = clock();

  arr(char) bwt =
      concatenated_arr(tree->super.buffers, tree->super.buffer_count);
  do_nothing(&bwt);
  arrfree(bwt);
  freeBinaryTreeBWT(tree);

  run->end = clock();
}

void bench_nary(struct benchmark_results *run) {
  run->start = clock();

  struct naryTreeBWT *tree = buildNaryTreeBWT(tree_depth, reduced_alphabet);

  run->prepare = clock();

  genericTreeBWT_compress(&tree->super, (const char **)reduced_inputs, lengths,
                          input_count);

  run->conclude = clock();

  arr(char) bwt =
      concatenated_arr(tree->super.buffers, tree->super.buffer_count);
  do_nothing(&bwt);
  arrfree(bwt);
  freeNaryTreeBWT(tree);

  run->end = clock();
}

void bench_huffman(struct benchmark_results *run) {
  run->start = clock();

  struct huffmanTreeBWT *tree =
      buildHuffmanTreeBWT(tree_depth, reduced_alphabet,
                          (const char **)reduced_inputs, lengths, input_count);

  run->prepare = clock();

  genericTreeBWT_compress(&tree->super, (const char **)reduced_inputs, lengths,
                          input_count);

  run->conclude = clock();

  arr(char) bwt =
      concatenated_arr(tree->super.buffers, tree->super.buffer_count);
  do_nothing(&bwt);
  arrfree(bwt);
  freeHuffmanTreeBWT(tree);

  run->end = clock();
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s FILE...\n", argv[0]);
    return 1;
  }

  // Read inputs
  input_count = argc - 1;
  inputs = malloc(input_count * sizeof(inputs[0]));
  for (int i = 1; i < argc; i++) {
    char *input = slurp(argv[i]);
    if (!input) {
      fprintf(stderr, "Could not read file \"%s\".", argv[i]);
      return 1;
    }

    inputs[i - 1] = input;
  }

  lengths = malloc(input_count * sizeof(lengths[0]));
  for (size_t i = 0; i < input_count; i++)
    lengths[i] = strlen(inputs[i]);

  reduced_inputs = malloc(input_count * sizeof(reduced_inputs[0]));
  for (size_t i = 0; i < input_count; i++)
    reduced_inputs[i] = malloc(lengths[i]);

  // Benchmark for different alphabet sizes and tree depths
  for (size_t p = 0; p < ARRLEN(params); p++) {
    reduced_alphabet.size = params[p].alphabet_size;
    printf("Benchmark alphabet size %i\n", reduced_alphabet.size);

    // Reduce inputs to alphabet size
    for (size_t input = 0; input < input_count; input++) {
      for (size_t j = 0; j < lengths[input]; j++) {
        unsigned char symbol = ((unsigned char *)inputs[input])[j];
        if (symbol == 0) {
          reduced_inputs[input][j] = '\0';
        } else {
          symbol = ((symbol - 1) % reduced_alphabet.size) + 1;
          reduced_inputs[input][j] = *(char *)&symbol;
        }
      }
    }

    // Run simple benchmark
    struct benchmark_results runs[RUN_COUNT];
    for (size_t i = 0; i < ARRLEN(runs); i++) {
      bench_simple(&runs[i]);
    }
    print_results("simple BWT", reduced_alphabet.size, 0, runs, ARRLEN(runs));
    fflush(stdout);

    // Run tree benchmarks for each tree depth
    for (int depth = 1; depth <= params[p].max_depth; depth++) {
      tree_depth = depth;
      printf("Benchmark depth %i\n", tree_depth);

      for (size_t i = 0; i < ARRLEN(runs); i++) {
        bench_binary(&runs[i]);
      }
      print_results("binary BWT", reduced_alphabet.size, tree_depth, runs, ARRLEN(runs));
      fflush(stdout);

      for (size_t i = 0; i < ARRLEN(runs); i++) {
        bench_nary(&runs[i]);
      }
      print_results("n-ary BWT", reduced_alphabet.size, tree_depth, runs, ARRLEN(runs));
      fflush(stdout);

      for (size_t i = 0; i < ARRLEN(runs); i++) {
        bench_huffman(&runs[i]);
      }
      print_results("huffman BWT", reduced_alphabet.size, tree_depth, runs, ARRLEN(runs));
      fflush(stdout);
    }
  }

  for (size_t i = 0; i < input_count; i++)
    free(reduced_inputs[i]);
  free(reduced_inputs);
  free(lengths);
  for (size_t i = 0; i < input_count; i++)
    free(inputs[i]);
  free(inputs);

  return 0;
}
