#include "merkle.h"
#include "crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int hash_transaction_leaf(const Transaction *tx, char *out, size_t out_size) {
    char payload[MAX_LINE_LEN];
    serialize_transaction_for_hash(tx, payload, sizeof(payload));
    return sha256_hex(payload, out, out_size);
}

int compute_merkle_root(Transaction *transactions, int count, char *root_out, size_t root_size) {
    if (!transactions || count <= 0 || !root_out) {
        return -1;
    }

    char *level = calloc((size_t)count, MAX_HASH_LEN);
    if (!level) {
        return -1;
    }

    for (int i = 0; i < count; i++) {
        if (hash_transaction_leaf(&transactions[i], level + i * MAX_HASH_LEN, MAX_HASH_LEN) != 0) {
            free(level);
            return -1;
        }
    }

    int level_count = count;
    char *current = level;
    char *next = calloc((size_t)count, MAX_HASH_LEN);
    if (!next) {
        free(level);
        return -1;
    }

    char *buf1 = level;
    char *buf2 = next;

    while (level_count > 1) {
        int next_count = 0;
        int pairs = level_count / 2;
        int has_odd = level_count % 2;

        for (int i = 0; i < pairs; i++) {
            char combined[MAX_LINE_LEN];
            snprintf(combined, sizeof(combined), "%s%s",
                     current + i * 2 * MAX_HASH_LEN,
                     current + (i * 2 + 1) * MAX_HASH_LEN);
            sha256_hex(combined, next + next_count * MAX_HASH_LEN, MAX_HASH_LEN);
            next_count++;
        }

        if (has_odd) {
            char combined[MAX_LINE_LEN];
            const char *last = current + (level_count - 1) * MAX_HASH_LEN;
            snprintf(combined, sizeof(combined), "%s%s", last, last);
            sha256_hex(combined, next + next_count * MAX_HASH_LEN, MAX_HASH_LEN);
            next_count++;
        }

        char *tmp = current;
        current = next;
        next = tmp;
        level_count = next_count;
    }

    strncpy(root_out, current, root_size - 1);
    root_out[root_size - 1] = '\0';
    free(buf1);
    free(buf2);
    return 0;
}

int verify_merkle_root(Transaction *transactions, int count, const char *expected_root) {
    char computed[MAX_HASH_LEN];
    if (compute_merkle_root(transactions, count, computed, sizeof(computed)) != 0) {
        return 0;
    }
    return strcmp(computed, expected_root) == 0;
}
