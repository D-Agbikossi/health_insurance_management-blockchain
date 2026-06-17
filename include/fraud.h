#ifndef FRAUD_H
#define FRAUD_H

#include "types.h"

int fraud_check(ChainState *state, const Transaction *tx, const char *member_id,
                const char *provider_id, double claim_amount);
int is_duplicate_tx_id(const ChainState *state, const char *tx_id);
void fraud_review(const ChainState *state);
int approve_suspicious(ChainState *state, const char *tx_id);
int reject_suspicious(ChainState *state, const char *tx_id);

#endif
