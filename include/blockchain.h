#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "types.h"

void chain_init(ChainState *state);
int blockchain_verify(ChainState *state);
void blockchain_view(const ChainState *state);
int apply_block_transactions(ChainState *state, Block *block);

#endif
