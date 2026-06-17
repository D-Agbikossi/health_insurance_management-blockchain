#include "utxo.h"
#include <stdio.h>
#include <string.h>

int utxo_create(ChainState *state, const char *owner, double amount, const char *origin_tx) {
    if (state->utxo_count >= MAX_UTXOS || amount <= 0) {
        return -1;
    }
    UTXO *u = &state->utxos[state->utxo_count++];
    snprintf(u->utxo_id, MAX_ID_LEN, "UTXO-%d", state->utxo_count);
    strncpy(u->owner_address, owner, MAX_ADDR_LEN - 1);
    u->amount = amount;
    u->spent = 0;
    strncpy(u->origin_tx_id, origin_tx, MAX_ID_LEN - 1);
    return 0;
}

int utxo_is_double_spend(const ChainState *state, const char *utxo_id) {
    for (int i = 0; i < state->utxo_count; i++) {
        if (strcmp(state->utxos[i].utxo_id, utxo_id) == 0) {
            return state->utxos[i].spent;
        }
    }
    return 1;
}

int utxo_validate(const ChainState *state, const char *utxo_id) {
    for (int i = 0; i < state->utxo_count; i++) {
        if (strcmp(state->utxos[i].utxo_id, utxo_id) == 0) {
            return !state->utxos[i].spent;
        }
    }
    return 0;
}

int utxo_spend_for_amount(ChainState *state, const char *owner, double amount,
                          char *spent_ids_out, size_t out_size) {
    double collected = 0;
    spent_ids_out[0] = '\0';

    for (int i = 0; i < state->utxo_count && collected < amount; i++) {
        UTXO *u = &state->utxos[i];
        if (u->spent || strcmp(u->owner_address, owner) != 0) {
            continue;
        }
        u->spent = 1;
        collected += u->amount;
        if (spent_ids_out[0]) {
            strncat(spent_ids_out, ",", out_size - strlen(spent_ids_out) - 1);
        }
        strncat(spent_ids_out, u->utxo_id, out_size - strlen(spent_ids_out) - 1);
    }

    if (collected < amount) {
        return -1;
    }

    double change = collected - amount;
    if (change > 0) {
        utxo_create(state, owner, change, "change");
    }
    return 0;
}

void utxo_view(const ChainState *state) {
    printf("\n=== UTXO Set ===\n");
    printf("%-12s %-20s %-12s %-8s %s\n", "UTXO ID", "Owner", "Amount", "Spent", "Origin TX");
    for (int i = 0; i < state->utxo_count; i++) {
        const UTXO *u = &state->utxos[i];
        printf("%-12s %-20s %-12.2f %-8s %s\n",
               u->utxo_id, u->owner_address, u->amount,
               u->spent ? "YES" : "NO", u->origin_tx_id);
    }
    printf("Total UTXOs: %d\n", state->utxo_count);
}
