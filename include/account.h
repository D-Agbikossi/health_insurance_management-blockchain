#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "types.h"

int account_transfer(ChainState *state, const char *from, const char *to,
                     double amount, Transaction *tx_out);
int account_validate_nonce(const ChainState *state, const char *address, uint64_t sender_nonce);
void account_increment_nonce(ChainState *state, const char *address);
int account_apply_transaction(ChainState *state, const Transaction *tx);
void account_view_balance(const ChainState *state, const char *address);
void account_init_system_wallets(ChainState *state);

#endif
