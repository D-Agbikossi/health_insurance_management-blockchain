#ifndef POLICY_H
#define POLICY_H

#include "types.h"

Member *find_member(ChainState *state, const char *member_id);
Provider *find_provider(ChainState *state, const char *provider_id);
Policy *find_policy(ChainState *state, const char *policy_id);
Claim *find_claim(ChainState *state, const char *claim_id);
int register_member(ChainState *state, const char *member_id, const char *name);
int enroll_policy(ChainState *state, const char *member_id, const char *policy_id,
                    const char *coverage_plan);
int renew_policy(ChainState *state, const char *policy_id);
int check_policy_valid_for_claim(ChainState *state, const char *policy_id);
void generate_claim_id(ChainState *state, char *id_out, size_t id_size);

#endif
