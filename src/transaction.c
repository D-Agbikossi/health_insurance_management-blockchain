#include "transaction.h"
#include "mempool.h"
#include "account.h"
#include "crypto.h"
#include "fraud.h"
#include "utxo.h"
#include <stdio.h>
#include <string.h>

void generate_tx_id(ChainState *state, char *id_out, size_t id_size) {
    state->tx_counter++;
    snprintf(id_out, id_size, "TX-%06d", state->tx_counter);
}

static void sign_system_tx(Transaction *tx) {
    snprintf(tx->digital_signature, MAX_SIG_LEN, "SYSTEM:signed");
}

static int is_system_pool_address(const char *addr) {
    return strcmp(addr, INSURANCE_POOL_ADDR) == 0 ||
           strcmp(addr, REINSURANCE_POOL_ADDR) == 0 ||
           strcmp(addr, SYSTEM_ADDR) == 0 ||
           strcmp(addr, MINER_POOL_ADDR) == 0;
}

static int tx_skips_sender_balance(TransactionType type) {
    return type == TX_CLAIM_SUBMISSION ||
           type == TX_CLAIM_APPROVAL ||
           type == TX_CLAIM_REJECTION ||
           type == TX_CLAIM_SETTLEMENT ||
           type == TX_POLICY_ENROLLMENT ||
           type == TX_POLICY_RENEWAL ||
           type == TX_SERVICE_REQUEST ||
           type == TX_PREAUTH_REQUEST ||
           type == TX_PREAUTH_APPROVE ||
           type == TX_MINING_REWARD ||
           type == TX_POOL_REWARD;
}

int create_and_sign_transaction(ChainState *state, Transaction *tx,
                                const char *sender_addr, const char *receiver_addr,
                                double amount, TransactionType type, double fee_hint) {
    (void)fee_hint;
    generate_tx_id(state, tx->transaction_id, sizeof(tx->transaction_id));
    strncpy(tx->sender_address, sender_addr, MAX_ADDR_LEN - 1);
    strncpy(tx->receiver_address, receiver_addr, MAX_ADDR_LEN - 1);
    tx->amount = amount;
    tx->transaction_type = type;
    tx->timestamp = time(NULL);

    Account *sender = find_account(state, sender_addr);
    uint64_t pending = 0;
    if (sender) {
        for (int i = 0; i < state->mempool_count; i++) {
            if (strcmp(state->mempool[i].sender, sender_addr) == 0) {
                pending++;
            }
        }
        tx->sender_nonce = sender->nonce + pending + 1;
    } else {
        tx->sender_nonce = 0;
    }

    if (sender && sender->private_key_pem[0] != '\0' && !is_system_pool_address(sender_addr)) {
        if (sign_transaction(tx, sender->private_key_pem) != 0) {
            return -1;
        }
    } else {
        sign_system_tx(tx);
    }
    return 0;
}

int validate_transaction_fields(const ChainState *state, const Transaction *tx) {
    int zero_amount_ok = (tx->transaction_type == TX_POLICY_ENROLLMENT ||
                          tx->transaction_type == TX_POLICY_RENEWAL ||
                          tx->transaction_type == TX_SERVICE_REQUEST ||
                          tx->transaction_type == TX_CLAIM_REJECTION);

    if (tx->amount < 0 || (!zero_amount_ok && tx->amount <= 0)) {
        printf("Validation failed: amount must be positive.\n");
        return 0;
    }

    if (is_duplicate_tx_id(state, tx->transaction_id)) {
        printf("Validation failed: duplicate transaction_id.\n");
        return 0;
    }

    Account *sender = find_account(state, tx->sender_address);
    if (sender) {
        uint64_t pending = 0;
        for (int i = 0; i < state->mempool_count; i++) {
            if (strcmp(state->mempool[i].sender, tx->sender_address) == 0) {
                pending++;
            }
        }
        uint64_t expected_nonce = sender->nonce + pending + 1;

        if (is_system_pool_address(tx->sender_address)) {
            if (strncmp(tx->digital_signature, "SYSTEM:", 7) != 0) {
                printf("Validation failed: invalid system signature.\n");
                return 0;
            }
        } else {
            if (tx->sender_nonce != expected_nonce) {
                printf("Validation failed: invalid sender_nonce (expected %llu, got %llu).\n",
                       (unsigned long long)expected_nonce,
                       (unsigned long long)tx->sender_nonce);
                return 0;
            }
            if (!verify_transaction_signature(tx, sender->public_key_hex)) {
                printf("Validation failed: invalid digital signature.\n");
                return 0;
            }
        }

        if (!tx_skips_sender_balance(tx->transaction_type) &&
            sender->balance < tx->amount &&
            !is_system_pool_address(tx->sender_address)) {
            printf("Validation failed: insufficient balance.\n");
            return 0;
        }
    } else if (strncmp(tx->digital_signature, "SYSTEM:", 7) != 0 &&
               tx->transaction_type != TX_MINING_REWARD &&
               tx->transaction_type != TX_POOL_REWARD &&
               tx->transaction_type != TX_CLAIM_SETTLEMENT) {
        if (strcmp(tx->sender_address, INSURANCE_POOL_ADDR) != 0 &&
            strcmp(tx->sender_address, REINSURANCE_POOL_ADDR) != 0 &&
            strcmp(tx->sender_address, SYSTEM_ADDR) != 0) {
            printf("Validation failed: unknown sender.\n");
            return 0;
        }
    }

    return 1;
}

void record_tx_history(ChainState *state, const Transaction *tx) {
    (void)state;
    (void)tx;
    /* History derived from blockchain blocks during audit commands */
}
