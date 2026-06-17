#include "block.h"
#include "crypto.h"
#include <stdio.h>
#include <string.h>

void compute_block_hash(const Block *block, char *hash_out, size_t hash_size) {
    char payload[MAX_LINE_LEN * 2];
    snprintf(payload, sizeof(payload),
             "%d|%ld|%d|%s|%s|%llu|%s|%d",
             block->block_id,
             (long)block->timestamp,
             block->transaction_count,
             block->previous_hash,
             block->merkle_root,
             (unsigned long long)block->nonce,
             block->miner_id,
             block->difficulty);
    sha256_hex(payload, hash_out, hash_size);
}

int verify_block_pow(const Block *block) {
    char hash[MAX_HASH_LEN];
    compute_block_hash(block, hash, sizeof(hash));
    return hash_meets_difficulty(hash, block->difficulty);
}

int verify_block_linkage(const Block *prev, const Block *current) {
    char prev_hash[MAX_HASH_LEN];
    compute_block_hash(prev, prev_hash, sizeof(prev_hash));
    return strcmp(prev_hash, current->previous_hash) == 0;
}
