#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "types.h"

void generate_tx_id(ChainState *state, char *id_out, size_t id_size);
int create_and_sign_transaction(ChainState *state, Transaction *tx,
                                const char *sender_addr, const char *receiver_addr,
                                double amount, TransactionType type, double fee_hint);
int validate_transaction_fields(const ChainState *state, const Transaction *tx);
void record_tx_history(ChainState *state, const Transaction *tx);

#endif
