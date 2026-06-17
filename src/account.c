#include "account.h"
#include "mempool.h"
#include "transaction.h"
#include "crypto.h"
#include "utxo.h"
#include <stdio.h>
#include <string.h>

Account *find_account(ChainState *state, const char *address) {
    for (int i = 0; i < state->account_count; i++) {
        if (strcmp(state->accounts[i].address, address) == 0) {
            return &state->accounts[i];
        }
    }
    return NULL;
}

Account *get_or_create_account(ChainState *state, const char *address) {
    Account *existing = find_account(state, address);
    if (existing) {
        return existing;
    }
    if (state->account_count >= MAX_ACCOUNTS) {
        return NULL;
    }
    Account *acc = &state->accounts[state->account_count++];
    memset(acc, 0, sizeof(*acc));
    strncpy(acc->address, address, MAX_ADDR_LEN - 1);
    return acc;
}

void account_init_system_wallets(ChainState *state) {
    Account *insurance = get_or_create_account(state, INSURANCE_POOL_ADDR);
    Account *reinsurance = get_or_create_account(state, REINSURANCE_POOL_ADDR);
    Account *miner = get_or_create_account(state, MINER_POOL_ADDR);
    Account *system = get_or_create_account(state, SYSTEM_ADDR);

    if (insurance && insurance->balance == 0) insurance->balance = 100000.0;
    if (reinsurance && reinsurance->balance == 0) reinsurance->balance = 50000.0;
    if (miner && miner->balance == 0) miner->balance = 0.0;
    if (system) system->balance = 0.0;

    strncpy(state->token.token_name, "ALU Health Token", MAX_NAME_LEN - 1);
    strncpy(state->token.token_symbol, "AHT", 7);
    state->token.total_supply = 1000000.0;
}

int account_validate_nonce(const ChainState *state, const char *address, uint64_t sender_nonce) {
    const Account *acc = NULL;
    for (int i = 0; i < state->account_count; i++) {
        if (strcmp(state->accounts[i].address, address) == 0) {
            acc = &state->accounts[i];
            break;
        }
    }
    uint64_t pending = 0;
    for (int i = 0; i < state->mempool_count; i++) {
        if (strcmp(state->mempool[i].sender, address) == 0 &&
            state->mempool[i].status != MEMPOOL_SUSPICIOUS) {
            pending++;
        }
    }
    uint64_t expected = (acc ? acc->nonce : 0) + pending + 1;
    return sender_nonce == expected;
}

void account_increment_nonce(ChainState *state, const char *address) {
    Account *acc = find_account(state, address);
    if (acc) {
        acc->nonce++;
    }
}

int account_apply_transaction(ChainState *state, const Transaction *tx) {
    int zero_amount_ok = (tx->transaction_type == TX_POLICY_ENROLLMENT ||
                          tx->transaction_type == TX_POLICY_RENEWAL ||
                          tx->transaction_type == TX_SERVICE_REQUEST ||
                          tx->transaction_type == TX_CLAIM_REJECTION);

    if (tx->amount < 0 || (!zero_amount_ok && tx->amount <= 0)) {
        return -1;
    }

    Account *sender = find_account(state, tx->sender_address);
    Account *receiver = find_account(state, tx->receiver_address);

    if (!receiver) {
        receiver = get_or_create_account(state, tx->receiver_address);
    }

    /* Mining rewards and system pool disbursements skip sender balance check */
    int skip_sender_debit = (tx->transaction_type == TX_MINING_REWARD ||
                             tx->transaction_type == TX_POOL_REWARD ||
                             tx->transaction_type == TX_CLAIM_SETTLEMENT ||
                             tx->transaction_type == TX_REINSURANCE_CONTRIBUTION ||
                             strcmp(tx->sender_address, SYSTEM_ADDR) == 0 ||
                             strcmp(tx->sender_address, INSURANCE_POOL_ADDR) == 0 ||
                             strcmp(tx->sender_address, REINSURANCE_POOL_ADDR) == 0);

    if (!skip_sender_debit) {
        if (!sender || sender->balance < tx->amount) {
            return -1;
        }
        if (tx->amount > 0) {
            sender->balance -= tx->amount;
        }
        account_increment_nonce(state, tx->sender_address);
    } else if (sender) {
        if (sender->balance >= tx->amount) {
            sender->balance -= tx->amount;
        } else if (strcmp(tx->sender_address, INSURANCE_POOL_ADDR) != 0 &&
                   strcmp(tx->sender_address, REINSURANCE_POOL_ADDR) != 0 &&
                   strcmp(tx->sender_address, SYSTEM_ADDR) != 0) {
            return -1;
        } else {
            sender->balance -= tx->amount;
            if (sender->balance < 0) {
                sender->balance = 0;
            }
        }
    }

    receiver->balance += tx->amount;
    return 0;
}

int account_transfer(ChainState *state, const char *from, const char *to,
                     double amount, Transaction *tx_out) {
    Account *sender = find_account(state, from);
    if (!sender || amount <= 0 || sender->balance < amount) {
        printf("Error: insufficient balance or invalid transfer.\n");
        return -1;
    }
    if (!account_validate_nonce(state, from, sender->nonce + 1)) {
        /* nonce set during create_and_sign_transaction */
    }

    Transaction tx;
    memset(&tx, 0, sizeof(tx));
    generate_tx_id(state, tx.transaction_id, sizeof(tx.transaction_id));
    strncpy(tx.sender_address, from, MAX_ADDR_LEN - 1);
    strncpy(tx.receiver_address, to, MAX_ADDR_LEN - 1);
    tx.amount = amount;
    tx.transaction_type = TX_ACCOUNT_TRANSFER;
    tx.timestamp = time(NULL);
    tx.sender_nonce = sender->nonce + 1;

    if (sign_transaction(&tx, sender->private_key_pem) != 0) {
        printf("Error: failed to sign transaction.\n");
        return -1;
    }

    if (tx_out) {
        *tx_out = tx;
    }
    return mempool_add(state, &tx, amount * 0.01);
}

void account_view_balance(const ChainState *state, const char *address) {
    const Account *acc = NULL;
    for (int i = 0; i < state->account_count; i++) {
        if (strcmp(state->accounts[i].address, address) == 0) {
            acc = &state->accounts[i];
            break;
        }
    }
    if (!acc) {
        printf("Account not found: %s\n", address);
        return;
    }
    printf("Address: %s\nBalance: %.2f AHT\nNonce: %llu\n",
           acc->address, acc->balance, (unsigned long long)acc->nonce);
}
