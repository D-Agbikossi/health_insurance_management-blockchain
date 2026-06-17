#ifndef MEMPOOL_H
#define MEMPOOL_H

#include "types.h"

void mempool_sort(ChainState *state);
int mempool_add(ChainState *state, const Transaction *tx, double fee);
int mempool_remove_confirmed(ChainState *state, Transaction *txs, int count);
MempoolEntry *mempool_find(ChainState *state, const char *tx_id);
int mempool_select_pending(ChainState *state, Transaction *out, int max_count);
void mempool_view(const ChainState *state);

#endif
