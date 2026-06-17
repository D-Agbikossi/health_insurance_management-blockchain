#include "blockchain.h"
#include "block.h"
#include "merkle.h"
#include "crypto.h"
#include "account.h"
#include "utxo.h"
#include <stdio.h>
#include <string.h>

void chain_init(ChainState *state) {
    memset(state, 0, sizeof(*state));
    state->current_difficulty = DIFFICULTY_DEFAULT;
    state->block_reward = BLOCK_REWARD_DEFAULT;
    state->last_retarget_block = 0;
    state->chain_verified = 0;
    state->tx_counter = 0;
    state->claim_counter = 0;

    account_init_system_wallets(state);

    Block genesis;
    memset(&genesis, 0, sizeof(genesis));
    genesis.block_id = 0;
    genesis.timestamp = time(NULL);
    genesis.transaction_count = 0;
    strncpy(genesis.previous_hash, "0", MAX_HASH_LEN - 1);
    strncpy(genesis.merkle_root, "0", MAX_HASH_LEN - 1);
    genesis.nonce = 0;
    strncpy(genesis.miner_id, SYSTEM_ADDR, MAX_ADDR_LEN - 1);
    genesis.difficulty = GENESIS_DIFFICULTY;
    compute_block_hash(&genesis, genesis.block_hash, MAX_HASH_LEN);

    state->blocks[0] = genesis;
    state->block_count = 1;
    strncpy(state->last_block_hash, genesis.block_hash, MAX_HASH_LEN - 1);
    state->chain_verified = 1;
}

int apply_block_transactions(ChainState *state, Block *block) {
    for (int i = 0; i < block->transaction_count; i++) {
        Transaction *tx = &block->transactions[i];
        account_apply_transaction(state, tx);

        if (tx->transaction_type == TX_PREMIUM_PAYMENT) {
            utxo_create(state, INSURANCE_POOL_ADDR, tx->amount * (1.0 - REINSURANCE_RATE),
                        tx->transaction_id);
        }
    }
    return 0;
}

int blockchain_verify(ChainState *state) {
    printf("\n=== Blockchain Verification ===\n");
    int errors = 0;

    for (int i = 0; i < state->block_count; i++) {
        Block *block = &state->blocks[i];

        if (i > 0) {
            if (!verify_block_linkage(&state->blocks[i - 1], block)) {
                printf("FAIL Block %d: previous_hash linkage broken.\n", i);
                errors++;
            }
            if (block->timestamp <= state->blocks[i - 1].timestamp) {
                printf("FAIL Block %d: timestamp not strictly increasing.\n", i);
                errors++;
            }
        }

        char recomputed[MAX_HASH_LEN];
        compute_block_hash(block, recomputed, sizeof(recomputed));
        if (strcmp(recomputed, block->block_hash) != 0) {
            printf("FAIL Block %d: block hash mismatch.\n", i);
            errors++;
        }

        if (i > 0 && !verify_block_pow(block)) {
            printf("FAIL Block %d: proof-of-work invalid.\n", i);
            errors++;
        }

        if (block->transaction_count > 0) {
            if (!verify_merkle_root(block->transactions, block->transaction_count,
                                    block->merkle_root)) {
                printf("FAIL Block %d: merkle root mismatch.\n", i);
                errors++;
            }
        }

        for (int t = 0; t < block->transaction_count; t++) {
            Transaction *tx = &block->transactions[t];
            Account *sender = find_account(state, tx->sender_address);
            if (sender && !verify_transaction_signature(tx, sender->public_key_hex)) {
                if (strncmp(tx->digital_signature, "SYSTEM:", 7) != 0) {
                    printf("FAIL Block %d TX %s: invalid signature.\n", i, tx->transaction_id);
                    errors++;
                }
            }
        }
    }

    if (errors == 0) {
        printf("All %d blocks verified successfully.\n", state->block_count);
        state->chain_verified = 1;
        return 0;
    }

    printf("Verification failed with %d error(s).\n", errors);
    state->chain_verified = 0;
    return -1;
}

void blockchain_view(const ChainState *state) {
    printf("\n=== Blockchain (%d blocks) ===\n", state->block_count);
    for (int i = 0; i < state->block_count; i++) {
        const Block *b = &state->blocks[i];
        printf("\nBlock #%d\n", b->block_id);
        printf("  Hash:          %s\n", b->block_hash);
        printf("  Previous:      %s\n", b->previous_hash);
        printf("  Merkle Root:   %s\n", b->merkle_root);
        printf("  Timestamp:     %s", ctime(&b->timestamp));
        printf("  Nonce:         %llu\n", (unsigned long long)b->nonce);
        printf("  Miner:         %s\n", b->miner_id);
        printf("  Difficulty:    %d\n", b->difficulty);
        printf("  Transactions:  %d\n", b->transaction_count);
        for (int t = 0; t < b->transaction_count; t++) {
            const Transaction *tx = &b->transactions[t];
            printf("    [%s] %s -> %s : %.2f AHT (%s)\n",
                   tx->transaction_id, tx->sender_address, tx->receiver_address,
                   tx->amount, tx_type_to_string(tx->transaction_type));
        }
    }
}
