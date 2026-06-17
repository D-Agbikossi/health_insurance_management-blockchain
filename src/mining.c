#include "mining.h"
#include "block.h"
#include "merkle.h"
#include "mempool.h"
#include "persistence.h"
#include "blockchain.h"
#include "transaction.h"
#include "account.h"
#include "crypto.h"
#include "utxo.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void difficulty_status(const ChainState *state) {
    printf("Current difficulty: %d\n", state->current_difficulty);
    printf("Last retarget at block: %d\n", state->last_retarget_block);
    printf("Block reward: %.2f AHT\n", state->block_reward);
}

void difficulty_retarget(ChainState *state) {
    if (state->block_count < RETARGET_INTERVAL + 1) {
        return;
    }
    if ((state->block_count - 1) % RETARGET_INTERVAL != 0) {
        return;
    }

    int start = state->block_count - RETARGET_INTERVAL;
    double total_time = 0;
    for (int i = start; i < state->block_count; i++) {
        total_time += difftime(state->blocks[i].timestamp, state->blocks[i - 1].timestamp);
    }
    double avg_time = total_time / RETARGET_INTERVAL;
    int old_diff = state->current_difficulty;
    int new_diff = old_diff;

    if (avg_time < MIN_RETARGET_TIME) {
        new_diff = old_diff + 1;
    } else if (avg_time > MAX_RETARGET_TIME) {
        new_diff = old_diff - 1;
        if (new_diff < 1) new_diff = 1;
    }

    state->current_difficulty = new_diff;
    state->last_retarget_block = state->block_count - 1;

    printf("\n=== Difficulty Retarget ===\n");
    printf("Old difficulty: %d\n", old_diff);
    printf("New difficulty: %d\n", new_diff);
    printf("Average block time: %.2f seconds\n\n", avg_time);
}

int mine_solo(ChainState *state, const char *miner_id, int max_tx) {
    if (!state->chain_verified && state->block_count > 0) {
        printf("Chain not verified. Run blockchain_verify first.\n");
        return -1;
    }

    Transaction selected[MAX_TX_PER_BLOCK];
    int count = mempool_select_pending(state, selected, max_tx > 0 ? max_tx : MAX_TX_PER_BLOCK);
    if (count == 0) {
        printf("No pending transactions to mine.\n");
        return -1;
    }

    Block block;
    memset(&block, 0, sizeof(block));
    block.block_id = state->block_count;
    block.timestamp = time(NULL);

    if (state->block_count > 0) {
        Block *prev = &state->blocks[state->block_count - 1];
        if (block.timestamp <= prev->timestamp) {
            block.timestamp = prev->timestamp + 1;
        }
        compute_block_hash(prev, block.previous_hash, MAX_HASH_LEN);
    } else {
        strncpy(block.previous_hash, "0", MAX_HASH_LEN - 1);
    }

    block.nonce = 0;
    strncpy(block.miner_id, miner_id, MAX_ADDR_LEN - 1);
    block.difficulty = state->current_difficulty;
    block.transaction_count = count;
    for (int i = 0; i < count; i++) {
        block.transactions[i] = selected[i];
    }

    printf("Mining block %d with %d transactions (difficulty=%d)...\n",
           block.block_id, count, block.difficulty);

    uint64_t attempts = 0;
    compute_merkle_root(selected, count, block.merkle_root, MAX_HASH_LEN);
    char hash[MAX_HASH_LEN];
    while (1) {
        attempts++;
        compute_block_hash(&block, hash, sizeof(hash));
        if (hash_meets_difficulty(hash, block.difficulty)) {
            strncpy(block.block_hash, hash, MAX_HASH_LEN - 1);
            break;
        }
        block.nonce++;
    }

    printf("Block mined after %llu hashes. Hash: %s\n",
           (unsigned long long)attempts, block.block_hash);

    if (state->block_count >= MAX_BLOCKS) return -1;
    state->blocks[state->block_count] = block;
    state->block_count++;
    strncpy(state->last_block_hash, block.block_hash, MAX_HASH_LEN - 1);

    apply_block_transactions(state, &block);
    mempool_remove_confirmed(state, selected, count);

    Transaction reward;
    create_and_sign_transaction(state, &reward, SYSTEM_ADDR, miner_id,
                                state->block_reward, TX_MINING_REWARD, 0);
    Account *miner = get_or_create_account(state, miner_id);
    if (miner) miner->balance += state->block_reward;
    printf("Mining reward: %.2f AHT to %s\n", state->block_reward, miner_id);

    difficulty_retarget(state);
    chain_save(state, PERSISTENCE_FILE);
    return 0;
}

int mine_pool(ChainState *state, PoolMiner *miners, int miner_count, int max_tx) {
    if (miner_count <= 0) {
        printf("No pool miners specified.\n");
        return -1;
    }

    Transaction selected[MAX_TX_PER_BLOCK];
    int count = mempool_select_pending(state, selected, max_tx > 0 ? max_tx : MAX_TX_PER_BLOCK);
    if (count == 0) {
        printf("No pending transactions to mine.\n");
        return -1;
    }

    Block block;
    memset(&block, 0, sizeof(block));
    block.block_id = state->block_count;
    block.timestamp = time(NULL);
    if (state->block_count > 0) {
        Block *prev = &state->blocks[state->block_count - 1];
        if (block.timestamp <= prev->timestamp) block.timestamp = prev->timestamp + 1;
        compute_block_hash(prev, block.previous_hash, MAX_HASH_LEN);
    } else {
        strncpy(block.previous_hash, "0", MAX_HASH_LEN - 1);
    }
    block.nonce = 0;
    strncpy(block.miner_id, "POOL", MAX_ADDR_LEN - 1);
    block.difficulty = state->current_difficulty;
    block.transaction_count = count;
    for (int i = 0; i < count; i++) {
        block.transactions[i] = selected[i];
    }

    compute_merkle_root(selected, count, block.merkle_root, MAX_HASH_LEN);

    uint64_t total_hashes = 0;
    int found = 0;
    for (int round = 0; !found; round++) {
        for (int m = 0; m < miner_count && !found; m++) {
            miners[m].hashes_attempted++;
            total_hashes++;
            compute_block_hash(&block, block.block_hash, MAX_HASH_LEN);
            if (hash_meets_difficulty(block.block_hash, block.difficulty)) {
                found = 1;
                printf("Pool block found by miner %s after %llu total hashes.\n",
                       miners[m].miner_id, (unsigned long long)total_hashes);
            } else {
                block.nonce++;
            }
        }
    }

    state->blocks[state->block_count] = block;
    state->block_count++;
    apply_block_transactions(state, &block);
    mempool_remove_confirmed(state, selected, count);

    uint64_t hash_sum = 0;
    for (int m = 0; m < miner_count; m++) hash_sum += miners[m].hashes_attempted;

    printf("\n=== Pool Reward Distribution ===\n");
    for (int m = 0; m < miner_count; m++) {
        double share = hash_sum > 0 ?
            (double)miners[m].hashes_attempted / (double)hash_sum : 0.0;
        double payout = state->block_reward * share;
        Transaction pool_tx;
        create_and_sign_transaction(state, &pool_tx, SYSTEM_ADDR, miners[m].miner_id,
                                    payout, TX_POOL_REWARD, 0);
        Account *acc = get_or_create_account(state, miners[m].miner_id);
        if (acc) acc->balance += payout;
        printf("Miner %s: %llu hashes (%.1f%%) -> %.4f AHT\n",
               miners[m].miner_id, (unsigned long long)miners[m].hashes_attempted,
               share * 100.0, payout);
    }

    difficulty_retarget(state);
    chain_save(state, PERSISTENCE_FILE);
    return 0;
}
