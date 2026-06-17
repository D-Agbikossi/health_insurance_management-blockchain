#ifndef REINSURANCE_H
#define REINSURANCE_H

#include "types.h"

int pay_premium_with_contribution(ChainState *state, const char *member_addr,
                                  double amount, double fee);
int settle_claim_with_reinsurance(ChainState *state, const char *claim_id);
double reinsurance_balance(const ChainState *state);

#endif
