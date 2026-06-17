#ifndef MERKLE_H
#define MERKLE_H

#include "types.h"

int compute_merkle_root(Transaction *transactions, int count, char *root_out, size_t root_size);
int verify_merkle_root(Transaction *transactions, int count, const char *expected_root);

#endif
