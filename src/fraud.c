#include "fraud.h"
#include "mempool.h"
#include "policy.h"
#include <stdio.h>
#include <string.h>

int is_duplicate_tx_id(const ChainState *state, const char *tx_id) {
    for (int i = 0; i < state->mempool_count; i++) {
        if (strcmp(state->mempool[i].transaction_id, tx_id) == 0) {
            return 1;
        }
    }
    for (int b = 0; b < state->block_count; b++) {
        const Block *block = &state->blocks[b];
        for (int t = 0; t < block->transaction_count; t++) {
            if (strcmp(block->transactions[t].transaction_id, tx_id) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

static int count_member_claims_24h(const ChainState *state, const char *member_id, time_t now) {
    time_t window_start = now - 86400;
    int count = 0;

    for (int c = 0; c < state->claim_count; c++) {
        if (strcmp(state->claims[c].member_id, member_id) != 0) continue;
        if (state->claims[c].submitted_at >= window_start) {
            count++;
        }
    }
    return count;
}

int fraud_check(ChainState *state, const Transaction *tx, const char *member_id,
                const char *provider_id, double claim_amount) {
    const char *reason = NULL;

    if (is_duplicate_tx_id(state, tx->transaction_id)) {
        reason = "Duplicate transaction_id";
    }

    time_t now = time(NULL);
    if (!reason && member_id && tx->transaction_type == TX_CLAIM_SUBMISSION) {
        /* Count existing claims; the current submission is not yet in the registry */
        if (count_member_claims_24h(state, member_id, now) >= 3) {
            reason = "High-frequency claims (>3 in 24h)";
        }
    }

    if (!reason && provider_id && claim_amount > 0) {
        Provider *prov = find_provider((ChainState *)state, provider_id);
        if (prov && prov->claim_count > 0) {
            double avg = prov->total_claims / prov->claim_count;
            if (claim_amount > 2.0 * avg) {
                reason = "Abnormal claim amount (>2x provider average)";
            }
        }
    }

    if (!reason) {
        return 0;
    }

    printf("FRAUD ALERT: Transaction %s flagged SUSPICIOUS - %s\n", tx->transaction_id, reason);
    return 1;
}

void fraud_review(const ChainState *state) {
    printf("\n=== Fraud Review ===\n");
    int found = 0;
    for (int i = 0; i < state->mempool_count; i++) {
        if (state->mempool[i].status == MEMPOOL_SUSPICIOUS) {
            found = 1;
            const MempoolEntry *e = &state->mempool[i];
            printf("%-12s %-16s %.2f AHT %-24s\n",
                   e->transaction_id, e->sender, e->amount,
                   tx_type_to_string(e->transaction_type));
        }
    }
    if (!found) {
        printf("No suspicious transactions pending review.\n");
    }
}

int approve_suspicious(ChainState *state, const char *tx_id) {
    MempoolEntry *entry = mempool_find(state, tx_id);
    if (!entry || entry->status != MEMPOOL_SUSPICIOUS) {
        printf("Transaction %s not found or not suspicious.\n", tx_id);
        return -1;
    }
    entry->status = MEMPOOL_PENDING;
    mempool_sort(state);
    printf("Transaction %s approved for mining.\n", tx_id);
    return 0;
}

int reject_suspicious(ChainState *state, const char *tx_id) {
    for (int i = 0; i < state->mempool_count; i++) {
        if (strcmp(state->mempool[i].transaction_id, tx_id) == 0) {
            printf("Transaction %s rejected and removed from mempool.\n", tx_id);
            for (int j = i; j < state->mempool_count - 1; j++) {
                state->mempool[j] = state->mempool[j + 1];
            }
            state->mempool_count--;
            return 0;
        }
    }
    printf("Transaction %s not found.\n", tx_id);
    return -1;
}
