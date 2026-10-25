#include "common.h"
#include "deps/greatest/greatest.h"

#include "binary-tree-bwt.h"
#include "bwt.h"
#include "huffman-tree-bwt.h"
#include "nary-tree-bwt.h"

/* include implementations for easier compilation */
#define STB_DS_IMPLEMENTATION
#include "deps/stb_ds.h"
#include "deps/timsort/timsort.c"

char fmt_buffer[256];

static int abc_map(char c, void *udata) {
  switch (c) {
  case 'a':
  case 'A':
    return 1;
  case 'b':
  case 'B':
    return 2;
  case 'c':
  case 'C':
    return 3;
  case '\0':
    return 0;
  }
  return -1;
}

static struct alphabet abc_alphabet = {
    .size = 3,
    .map = abc_map,
    .udata = NULL,
};

static int acgt_map(char c, void *udata) { return acgt_idx(c); }

static struct alphabet acgt_alphabet = {
    .size = 4,
    .map = acgt_map,
    .udata = NULL,
};

static int default_map(char c, void *udata) { return *(unsigned char *)&c; }

static struct alphabet default_alphabet = {
    .size = 255,
    .map = default_map,
    .udata = NULL,
};

struct {
  struct alphabet *alphabet;
  const char *in;
  struct {
    const char *buf;
    size_t len;
  } exp;
} test_simple_inputs[] = {
    {&default_alphabet, "abraca", {"ac\0raab", 7}},
    {&default_alphabet, "ABRACA", {"AC\0RAAB", 7}},
    {&default_alphabet, "abracadabra", {"ard\0rcaaaabb", 12}},
    {&default_alphabet, "abddc", {"c\0addb", 6}},
    {&acgt_alphabet, "TAGACCTC", {"CGTTACA\0C", 9}},
    {&acgt_alphabet, "CAATT", {"TCA\0TA", 6}},
    {&acgt_alphabet, "CAT", {"TC\0A", 4}},
};

/* Tests whether the input string is at index 0 in the conversion */
TEST bwt_should_begin_with_last_char(void) {
  for (size_t i = 0; i < ARRLEN(test_simple_inputs); i++) {
    size_t len = strlen(test_simple_inputs[i].in);
    arr(char) bwt =
        bwt_compress((const char *[]){test_simple_inputs[i].in},
                     (size_t[]){len}, 1, test_simple_inputs[i].alphabet);

    char exp = test_simple_inputs[i].in[len - 1];
    char got = bwt[0];
    snprintf(fmt_buffer, ARRLEN(fmt_buffer), "'%c' != '%c'", exp, got);
    ASSERT_EQ_FMTm(fmt_buffer, exp, got, "%c");

    arrfree(bwt);
  }
  PASS();
}

/* Tests the conversion of a single input string */
TEST simple_conversion(void) {
  for (size_t i = 0; i < ARRLEN(test_simple_inputs); i++) {
    arr(char) bwt = bwt_compress((const char *[]){test_simple_inputs[i].in},
                                 (size_t[]){strlen(test_simple_inputs[i].in)},
                                 1, test_simple_inputs[i].alphabet);

    ASSERT_EQ(test_simple_inputs[i].exp.len, arrlenu(bwt));
    ASSERT_MEM_EQ(test_simple_inputs[i].exp.buf, bwt, arrlen(bwt));

    arrfree(bwt);
  }
  PASS();
}

TEST simple_roundtrip(void) {
  for (size_t i = 0; i < ARRLEN(test_simple_inputs); i++) {
    arr(char) bwt = bwt_compress((const char *[]){test_simple_inputs[i].in},
                                 (size_t[]){strlen(test_simple_inputs[i].in)},
                                 1, test_simple_inputs[i].alphabet);

    arr(char) dec = bwt_decompress(bwt, 0, test_simple_inputs[i].alphabet);

    ASSERT_EQ(strlen(test_simple_inputs[i].in), arrlenu(dec));
    ASSERT_MEM_EQ(test_simple_inputs[i].in, dec, arrlen(dec));

    arrfree(dec);
    arrfree(bwt);
  }

  PASS();
}

SUITE(simple_suite) {
  RUN_TEST(bwt_should_begin_with_last_char);
  RUN_TEST(simple_conversion);
  RUN_TEST(simple_roundtrip);
}

struct {
  struct alphabet *alphabet;
  struct {
    size_t count;
    const char **inputs;
  } in;
  const char *exp;
} test_multiple_inputs[] = {
    {
        &acgt_alphabet,
        {3, (const char *[]){"C", "AT", "ACG"}},
        "CTG\0\0\0ACA",
    },
    {
        &acgt_alphabet,
        {5, (const char *[]){"CT", "ACTG", "TT", "CTAAA", "ACTTG"}},
        "TGTAGAAT\0\0\0\0AATTCTCCT\0C",
    },
	{
		&abc_alphabet,
		{3, (const char *[]){"b", "abc", "ac"}},
		"bcc\0\0\0aba",
	},
};

TEST multi_conversion(void) {
  for (size_t i = 0; i < ARRLEN(test_multiple_inputs); i++) {
    const char **inputs = test_multiple_inputs[i].in.inputs;
    size_t count = test_multiple_inputs[i].in.count;
    size_t lengths[count];
    int explen = 0;

    for (size_t j = 0; j < count; j++) {
      size_t length = strlen(inputs[j]);
      lengths[j] = length;
      explen += length + 1; // transformed input length plus terminator
    }

    arr(char) bwt =
        bwt_compress(inputs, lengths, count, test_multiple_inputs[i].alphabet);

    ASSERT_EQ(explen, arrlen(bwt));
    ASSERT_MEM_EQ(test_multiple_inputs[i].exp, bwt, arrlen(bwt));

    arrfree(bwt);
  }

  PASS();
}

TEST multi_roundtrip(void) {
  for (size_t i = 0; i < ARRLEN(test_multiple_inputs); i++) {
    const char **inputs = test_multiple_inputs[i].in.inputs;
    size_t count = test_multiple_inputs[i].in.count;
    size_t lengths[count];
    for (size_t j = 0; j < count; j++) {
      lengths[j] = strlen(inputs[j]);
    }

    arr(char) bwt =
        bwt_compress(inputs, lengths, count, test_multiple_inputs[i].alphabet);

    for (size_t j = 0; j < count; j++) {
      arr(char) dec = bwt_decompress(bwt, j, test_multiple_inputs[i].alphabet);
      ASSERT_EQ(strlen(inputs[j]), arrlenu(dec));
      ASSERT_MEM_EQ(inputs[j], dec, arrlen(dec));
      arrfree(dec);
    }

    arrfree(bwt);
  }

  PASS();
}

SUITE(multi_suite) {
  RUN_TEST(multi_conversion);
  RUN_TEST(multi_roundtrip);
}

/* Tests a single string conversion with the binary tree */
TEST binaryTreeBWT_single_input_test(void) {
  for (size_t i = 0; i < ARRLEN(test_simple_inputs); i++) {
    struct binaryTreeBWT *tree =
        buildBinaryTreeBWT(2, *test_simple_inputs[i].alphabet);
    genericTreeBWT_compress(&tree->super,
                            (const char *[]){test_simple_inputs[i].in},
                            (size_t[]){strlen(test_simple_inputs[i].in)}, 1);
    arr(char) bwt =
        concatenated_arr(tree->super.buffers, tree->super.buffer_count);

    ASSERT_EQ(test_simple_inputs[i].exp.len, arrlenu(bwt));
    ASSERT_MEM_EQ(test_simple_inputs[i].exp.buf, bwt, arrlen(bwt));

    arrfree(bwt);
    freeBinaryTreeBWT(tree);
  }

  PASS();
}

/* Tests multiple string conversion with the binary tree */
TEST binaryTreeBWT_multiple_input_test(void) {
  for (size_t i = 0; i < ARRLEN(test_multiple_inputs); i++) {
    const char **inputs = test_multiple_inputs[i].in.inputs;
    size_t count = test_multiple_inputs[i].in.count;
    size_t lengths[count];
    int explen = 0;

    for (size_t j = 0; j < count; j++) {
      size_t length = strlen(inputs[j]);
      lengths[j] = length;
      explen += length + 1; // transformed input length plus terminator
    }

    struct binaryTreeBWT *tree =
        buildBinaryTreeBWT(2, *test_multiple_inputs[i].alphabet);
    genericTreeBWT_compress(&tree->super, inputs, lengths, count);
    arr(char) bwt =
        concatenated_arr(tree->super.buffers, tree->super.buffer_count);

    ASSERT_EQ(explen, arrlen(bwt));
    ASSERT_MEM_EQ(test_multiple_inputs[i].exp, bwt, arrlen(bwt));

    arrfree(bwt);
    freeBinaryTreeBWT(tree);
  }

  PASS();
}

SUITE(binaryTreeBWT_suite) {
  RUN_TEST(binaryTreeBWT_single_input_test);
  RUN_TEST(binaryTreeBWT_multiple_input_test);
}

/* Tests a single string conversion with the n-ary tree */
TEST naryTreeBWT_single_input_test(void) {
  for (size_t i = 0; i < ARRLEN(test_simple_inputs); i++) {
    struct naryTreeBWT *tree =
        buildNaryTreeBWT(2, *test_simple_inputs[i].alphabet);
    genericTreeBWT_compress(&tree->super,
                            (const char *[]){test_simple_inputs[i].in},
                            (size_t[]){strlen(test_simple_inputs[i].in)}, 1);
    arr(char) bwt =
        concatenated_arr(tree->super.buffers, tree->super.buffer_count);

    ASSERT_EQ(test_simple_inputs[i].exp.len, arrlenu(bwt));
    ASSERT_MEM_EQ(test_simple_inputs[i].exp.buf, bwt, arrlen(bwt));

    arrfree(bwt);
    freeNaryTreeBWT(tree);
  }

  PASS();
}

/* Tests multiple string conversion with the n-ary tree */
TEST naryTreeBWT_multiple_input_test(void) {
  for (size_t i = 0; i < ARRLEN(test_multiple_inputs); i++) {
    const char **inputs = test_multiple_inputs[i].in.inputs;
    size_t count = test_multiple_inputs[i].in.count;
    size_t lengths[count];
    int explen = 0;

    for (size_t j = 0; j < count; j++) {
      size_t length = strlen(inputs[j]);
      lengths[j] = length;
      explen += length + 1; // transformed input length plus terminator
    }

    struct naryTreeBWT *tree =
        buildNaryTreeBWT(2, *test_multiple_inputs[i].alphabet);
    genericTreeBWT_compress(&tree->super, inputs, lengths, count);
    arr(char) bwt =
        concatenated_arr(tree->super.buffers, tree->super.buffer_count);

    ASSERT_EQ(explen, arrlen(bwt));
    ASSERT_MEM_EQ(test_multiple_inputs[i].exp, bwt, arrlen(bwt));

    arrfree(bwt);
    freeNaryTreeBWT(tree);
  }

  PASS();
}

SUITE(naryTreeBWT_suite) {
  RUN_TEST(naryTreeBWT_single_input_test);
  RUN_TEST(naryTreeBWT_multiple_input_test);
}

/* Tests multiple string conversion with the huffman tree */
TEST huffmanTreeBWT_single_input_test(void) {
  for (size_t i = 0; i < ARRLEN(test_simple_inputs); i++) {
    const char *inputs[] = {test_simple_inputs[i].in};
    size_t lengths[] = {strlen(test_simple_inputs[i].in)};
    size_t count = 1;

    struct huffmanTreeBWT *tree = buildHuffmanTreeBWT(
        2, *test_simple_inputs[i].alphabet, inputs, lengths, count);
    genericTreeBWT_compress(&tree->super, inputs, lengths, count);
    arr(char) bwt =
        concatenated_arr(tree->super.buffers, tree->super.buffer_count);

    ASSERT_EQ(test_simple_inputs[i].exp.len, arrlenu(bwt));
    ASSERT_MEM_EQ(test_simple_inputs[i].exp.buf, bwt, arrlen(bwt));

    arrfree(bwt);
    freeHuffmanTreeBWT(tree);
  }

  PASS();
}

/* Tests a single string conversion with the huffman tree */
TEST huffmanTreeBWT_multiple_input_test(void) {
  for (size_t i = 0; i < ARRLEN(test_multiple_inputs); i++) {
    const char **inputs = test_multiple_inputs[i].in.inputs;
    size_t count = test_multiple_inputs[i].in.count;
    size_t lengths[count];
    int explen = 0;

    for (size_t j = 0; j < count; j++) {
      size_t length = strlen(inputs[j]);
      lengths[j] = length;
      explen += length + 1; // transformed input length plus terminator
    }

    struct huffmanTreeBWT *tree = buildHuffmanTreeBWT(
        2, *test_multiple_inputs[i].alphabet, inputs, lengths, count);
    genericTreeBWT_compress(&tree->super, inputs, lengths, count);

    arr(char) bwt =
        concatenated_arr(tree->super.buffers, tree->super.buffer_count);

    ASSERT_EQ(explen, arrlen(bwt));
    ASSERT_MEM_EQ(test_multiple_inputs[i].exp, bwt, arrlen(bwt));

    arrfree(bwt);
    freeHuffmanTreeBWT(tree);
  }

  PASS();
}

SUITE(huffmanTreeBWT_suite) {
  RUN_TEST(huffmanTreeBWT_single_input_test);
  RUN_TEST(huffmanTreeBWT_multiple_input_test);
}

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[]) {
  GREATEST_MAIN_BEGIN();

  RUN_SUITE(simple_suite);
  RUN_SUITE(multi_suite);
  RUN_SUITE(binaryTreeBWT_suite);
  RUN_SUITE(naryTreeBWT_suite);
  RUN_SUITE(huffmanTreeBWT_suite);

  GREATEST_MAIN_END();
}
