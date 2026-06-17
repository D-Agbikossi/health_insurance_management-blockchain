#include "mempool.h"
#include "transaction.h"
#include <stdio.h>
#include <string.h>

static int mempool_cmp(const MempoolEntry *a, const MempoolEntry *b) {
    if (a->fee > b->fee) return -1;
    if (a->fee < b->fee) return 1;
    if (a->tx.timestamp < b->tx.timestamp) return -1;
    if (a->tx.timestamp > b->tx.timestamp) return 1;
    return 0;
}

void mempool_sort(ChainState *state) {
    for (int i = 0; i < state->mempool_count - 1; i++) {
        for (int j = i + 1; j < state->mempool_count; j++) {
            if (mempool_cmp(&state->mempool[i], &state->mempool[j]) > 0) {
                MempoolEntry tmp = state->mempool[i];
                state->mempool[i] = state->mempool[j];
                state->mempool[j] = tmp;
            }
        }
    }
}

int mempool_add(ChainState *state, const Transaction *tx, double fee) {
    if (!validate_transaction_fields(state, tx)) {
        return -1;
    }
    if (state->mempool_count >= MAX_MEMPOOL) {
        printf("Mempool full.\n");
        return -1;
    }

    MempoolEntry *entry = &state->mempool[state->mempool_count++];
    memset(entry, 0, sizeof(*entry));
    strncpy(entry->transaction_id, tx->transaction_id, MAX_ID_LEN - 1);
    strncpy(entry->sender, tx->sender_address, MAX_ADDR_LEN - 1);
    strncpy(entry->receiver, tx->receiver_address, MAX_ADDR_LEN - 1);
    entry->amount = tx->amount;
    entry->transaction_type = tx->transaction_type;
    entry->fee = fee;
    entry->status = MEMPOOL_PENDING;
    entry->tx = *tx;

    mempool_sort(state);
    printf("Transaction %s added to mempool (fee=%.2f, status=PENDING).\n",
           tx->transaction_id, fee);
    return 0;
}

MempoolEntry *mempool_find(ChainState *state, const char *tx_id) {
    for (int i = 0; i < state->mempool_count; i++) {
        if (strcmp(state->mempool[i].transaction_id, tx_id) == 0) {
            return &state->mempool[i];
        }
    }
    return NULL;
}

int mempool_remove_confirmed(ChainState *state, Transaction *txs, int count) {
    for (int t = 0; t < count; t++) {
        for (int i = 0; i < state->mempool_count; i++) {
            if (strcmp(state->mempool[i].transaction_id, txs[t].transaction_id) == 0) {
                for (int j = i; j < state->mempool_count - 1; j++) {
                    state->mempool[j] = state->mempool[j + 1];
                }
                state->mempool_count--;
                i--;
            }
        }
    }
    return 0;
}

int mempool_select_pending(ChainState *state, Transaction *out, int max_count) {
    mempool_sort(state);
    int selected = 0;
    for (int i = 0; i < state->mempool_count && selected < max_count; i++) {
        if (state->mempool[i].status == MEMPOOL_PENDING) {
            out[selected++] = state->mempool[i].tx;
        }
    }
    return selected;
}

void mempool_view(const ChainState *state) {
    printf("\n=== Mempool (%d entries) ===\n", state->mempool_count);
    printf("%-12s %-16s %-16s %-10s %-24s %-8s %s\n",
           "TX ID", "Sender", "Receiver", "Amount", "Type", "Fee", "Status");
    for (int i = 0; i < state->mempool_count; i++) {
        const MempoolEntry *e = &state->mempool[i];
        printf("%-12s %-16s %-16s %-10.2f %-24s %-8.2f %s\n",
               e->transaction_id,
               e->sender,
               e->receiver,
               e->amount,
               tx_type_to_string(e->transaction_type),
               e->fee,
               mempool_status_to_string(e->status));
    }
}
