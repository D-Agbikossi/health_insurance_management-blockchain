#ifndef BLOCK_H
#define BLOCK_H

#include "types.h"

void compute_block_hash(const Block *block, char *hash_out, size_t hash_size);
int verify_block_pow(const Block *block);
int verify_block_linkage(const Block *prev, const Block *current);

#endif
