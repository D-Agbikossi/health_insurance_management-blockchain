#ifndef UTXO_H
#define UTXO_H

#include "types.h"

Account *find_account(ChainState *state, const char *address);
Account *get_or_create_account(ChainState *state, const char *address);
int utxo_create(ChainState *state, const char *owner, double amount, const char *origin_tx);
int utxo_spend_for_amount(ChainState *state, const char *owner, double amount,
                          char *spent_ids_out, size_t out_size);
int utxo_validate(const ChainState *state, const char *utxo_id);
void utxo_view(const ChainState *state);
int utxo_is_double_spend(const ChainState *state, const char *utxo_id);

#endif
