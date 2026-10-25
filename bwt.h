#ifndef BWT_H
#define BWT_H

#include "common.h"

/**
 * Transforms multiple inputs into one Burrows Wheeler Transform
 *
 * @param  inputs   the array of input strings
 * @param  lengths  the lengths of each input string
 * @param  count    the number of input strings
 * @param  alphabet the optional alphabet of symbols, uses a 2^8 = 256 alphabet
 *                  if NULL
 * @return          the Burrows Wheeler Transform of the inputs
 */
arr(char) bwt_compress(const char *inputs[], size_t lengths[], size_t count,
                       struct alphabet *alphabet);

/**
 * Decompresses a Burrows Wheeler Transform.
 *
 * @param  bwt      the burrows wheeler transform
 * @param  alphabet the optional alphabet of symbols, uses a 2^8 = 256 alphabet
 *                  if NULL
 * @param  index    the index of the string within the burrows wheeler transform
 *                  to decompress
 * @return          the decompressed string
 */
arr(char)
    bwt_decompress(arr(char) bwt, size_t index, struct alphabet *alphabet);

#endif
