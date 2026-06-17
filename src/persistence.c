#include "persistence.h"
#include <stdio.h>
#include <string.h>

#define MAGIC "AHTCHAIN1"

static int write_string(FILE *f, const char *s) {
    size_t len = s ? strlen(s) : 0;
    if (fwrite(&len, sizeof(len), 1, f) != 1) return -1;
    if (len > 0 && fwrite(s, 1, len, f) != len) return -1;
    return 0;
}

static int read_string(FILE *f, char *buf, size_t buf_size) {
    size_t len = 0;
    if (fread(&len, sizeof(len), 1, f) != 1) return -1;
    if (len >= buf_size) return -1;
    if (len > 0 && fread(buf, 1, len, f) != len) return -1;
    buf[len] = '\0';
    return 0;
}

int chain_save(const ChainState *state, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        printf("Failed to save chain to %s\n", filename);
        return -1;
    }

    fwrite(MAGIC, 1, strlen(MAGIC), f);

    fwrite(&state->current_difficulty, sizeof(int), 1, f);
    fwrite(&state->block_reward, sizeof(double), 1, f);
    fwrite(&state->last_retarget_block, sizeof(int), 1, f);
    fwrite(&state->block_count, sizeof(int), 1, f);
    fwrite(&state->mempool_count, sizeof(int), 1, f);
    fwrite(&state->utxo_count, sizeof(int), 1, f);
    fwrite(&state->account_count, sizeof(int), 1, f);
    fwrite(&state->member_count, sizeof(int), 1, f);
    fwrite(&state->provider_count, sizeof(int), 1, f);
    fwrite(&state->policy_count, sizeof(int), 1, f);
    fwrite(&state->claim_count, sizeof(int), 1, f);
    fwrite(&state->tx_counter, sizeof(int), 1, f);
    fwrite(&state->claim_counter, sizeof(int), 1, f);
    fwrite(&state->token, sizeof(Token), 1, f);
    write_string(f, state->last_block_hash);

    for (int i = 0; i < state->block_count; i++) {
        const Block *b = &state->blocks[i];
        fwrite(b, sizeof(Block), 1, f);
    }
    for (int i = 0; i < state->mempool_count; i++) {
        fwrite(&state->mempool[i], sizeof(MempoolEntry), 1, f);
    }
    for (int i = 0; i < state->utxo_count; i++) {
        fwrite(&state->utxos[i], sizeof(UTXO), 1, f);
    }
    for (int i = 0; i < state->account_count; i++) {
        fwrite(&state->accounts[i], sizeof(Account), 1, f);
    }
    for (int i = 0; i < state->member_count; i++) {
        fwrite(&state->members[i], sizeof(Member), 1, f);
    }
    for (int i = 0; i < state->provider_count; i++) {
        fwrite(&state->providers[i], sizeof(Provider), 1, f);
    }
    for (int i = 0; i < state->policy_count; i++) {
        fwrite(&state->policies[i], sizeof(Policy), 1, f);
    }
    for (int i = 0; i < state->claim_count; i++) {
        fwrite(&state->claims[i], sizeof(Claim), 1, f);
    }

    fclose(f);
    printf("Chain state saved to %s\n", filename);
    return 0;
}

int chain_load(ChainState *state, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        return -1;
    }

    char magic[16] = {0};
    if (fread(magic, 1, strlen(MAGIC), f) != strlen(MAGIC) ||
        strcmp(magic, MAGIC) != 0) {
        fclose(f);
        return -1;
    }

    memset(state, 0, sizeof(*state));
    state->chain_verified = 0;

    fread(&state->current_difficulty, sizeof(int), 1, f);
    fread(&state->block_reward, sizeof(double), 1, f);
    fread(&state->last_retarget_block, sizeof(int), 1, f);
    fread(&state->block_count, sizeof(int), 1, f);
    fread(&state->mempool_count, sizeof(int), 1, f);
    fread(&state->utxo_count, sizeof(int), 1, f);
    fread(&state->account_count, sizeof(int), 1, f);
    fread(&state->member_count, sizeof(int), 1, f);
    fread(&state->provider_count, sizeof(int), 1, f);
    fread(&state->policy_count, sizeof(int), 1, f);
    fread(&state->claim_count, sizeof(int), 1, f);
    fread(&state->tx_counter, sizeof(int), 1, f);
    fread(&state->claim_counter, sizeof(int), 1, f);
    fread(&state->token, sizeof(Token), 1, f);
    read_string(f, state->last_block_hash, MAX_HASH_LEN);

    for (int i = 0; i < state->block_count; i++) {
        fread(&state->blocks[i], sizeof(Block), 1, f);
    }
    for (int i = 0; i < state->mempool_count; i++) {
        fread(&state->mempool[i], sizeof(MempoolEntry), 1, f);
    }
    for (int i = 0; i < state->utxo_count; i++) {
        fread(&state->utxos[i], sizeof(UTXO), 1, f);
    }
    for (int i = 0; i < state->account_count; i++) {
        fread(&state->accounts[i], sizeof(Account), 1, f);
    }
    for (int i = 0; i < state->member_count; i++) {
        fread(&state->members[i], sizeof(Member), 1, f);
    }
    for (int i = 0; i < state->provider_count; i++) {
        fread(&state->providers[i], sizeof(Provider), 1, f);
    }
    for (int i = 0; i < state->policy_count; i++) {
        fread(&state->policies[i], sizeof(Policy), 1, f);
    }
    for (int i = 0; i < state->claim_count; i++) {
        fread(&state->claims[i], sizeof(Claim), 1, f);
    }

    fclose(f);
    printf("Chain state loaded from %s (%d blocks).\n", filename, state->block_count);
    printf("Run blockchain_verify before further operations.\n");
    return 0;
}
