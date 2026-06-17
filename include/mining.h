#ifndef MINING_H
#define MINING_H

#include "types.h"

typedef struct {
    char miner_id[MAX_ADDR_LEN];
    uint64_t hashes_attempted;
} PoolMiner;

int mine_solo(ChainState *state, const char *miner_id, int max_tx);
int mine_pool(ChainState *state, PoolMiner *miners, int miner_count, int max_tx);
void difficulty_retarget(ChainState *state);
void difficulty_status(const ChainState *state);

#endif
