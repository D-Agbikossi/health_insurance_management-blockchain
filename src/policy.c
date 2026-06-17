#include "policy.h"
#include "account.h"
#include "crypto.h"
#include "mempool.h"
#include "transaction.h"
#include <stdio.h>
#include <string.h>

Member *find_member(ChainState *state, const char *member_id) {
    for (int i = 0; i < state->member_count; i++) {
        if (strcmp(state->members[i].member_id, member_id) == 0) {
            return &state->members[i];
        }
    }
    return NULL;
}

Provider *find_provider(ChainState *state, const char *provider_id) {
    for (int i = 0; i < state->provider_count; i++) {
        if (strcmp(state->providers[i].provider_id, provider_id) == 0) {
            return &state->providers[i];
        }
    }
    return NULL;
}

Policy *find_policy(ChainState *state, const char *policy_id) {
    for (int i = 0; i < state->policy_count; i++) {
        if (strcmp(state->policies[i].policy_id, policy_id) == 0) {
            return &state->policies[i];
        }
    }
    return NULL;
}

Claim *find_claim(ChainState *state, const char *claim_id) {
    for (int i = 0; i < state->claim_count; i++) {
        if (strcmp(state->claims[i].claim_id, claim_id) == 0) {
            return &state->claims[i];
        }
    }
    return NULL;
}

void generate_claim_id(ChainState *state, char *id_out, size_t id_size) {
    state->claim_counter++;
    snprintf(id_out, id_size, "CLM-%06d", state->claim_counter);
}

int register_member(ChainState *state, const char *member_id, const char *name) {
    if (find_member(state, member_id)) {
        printf("Member %s already registered.\n", member_id);
        return -1;
    }
    if (state->member_count >= MAX_MEMBERS) {
        return -1;
    }

    Member *m = &state->members[state->member_count++];
    strncpy(m->member_id, member_id, MAX_ID_LEN - 1);
    strncpy(m->name, name, MAX_NAME_LEN - 1);
    m->registered = 1;

    Account *wallet = &state->accounts[state->account_count];
    memset(wallet, 0, sizeof(*wallet));
    generate_keypair(wallet);
    state->account_count++;
    strncpy(m->wallet_address, wallet->address, MAX_ADDR_LEN - 1);
    wallet->balance = 1000.0;

    printf("Member registered: %s (%s)\n", member_id, name);
    printf("Wallet address: %s\n", m->wallet_address);
    printf("Initial balance: 1000.00 AHT\n");
    return 0;
}

int enroll_policy(ChainState *state, const char *member_id, const char *policy_id,
                    const char *coverage_plan) {
    Member *member = find_member(state, member_id);
    if (!member) {
        printf("Error: member %s not found.\n", member_id);
        return -1;
    }
    if (find_policy(state, policy_id)) {
        printf("Error: policy %s already exists.\n", policy_id);
        return -1;
    }
    if (state->policy_count >= MAX_POLICIES) {
        return -1;
    }

    Policy *p = &state->policies[state->policy_count++];
    strncpy(p->policy_id, policy_id, MAX_ID_LEN - 1);
    strncpy(p->member_id, member_id, MAX_ID_LEN - 1);
    strncpy(p->coverage_plan, coverage_plan, MAX_PLAN_LEN - 1);
    p->enrollment_date = time(NULL);
    p->expiry_date = p->enrollment_date + POLICY_DURATION_SEC;
    p->status = POLICY_ACTIVE;

    Transaction tx;
    create_and_sign_transaction(state, &tx, member->wallet_address, INSURANCE_POOL_ADDR,
                                  0.0, TX_POLICY_ENROLLMENT, 0.0);
    mempool_add(state, &tx, 1.0);

    printf("Policy enrolled: %s for member %s (plan: %s)\n", policy_id, member_id, coverage_plan);
    printf("Expiry date: %s", ctime(&p->expiry_date));
    return 0;
}

int renew_policy(ChainState *state, const char *policy_id) {
    Policy *p = find_policy(state, policy_id);
    if (!p) {
        printf("Policy not found: %s\n", policy_id);
        return -1;
    }
    Member *m = find_member(state, p->member_id);
    if (!m) {
        return -1;
    }

    p->expiry_date = time(NULL) + POLICY_DURATION_SEC;
    p->status = POLICY_RENEWED;

    Transaction tx;
    create_and_sign_transaction(state, &tx, m->wallet_address, INSURANCE_POOL_ADDR,
                                  0.0, TX_POLICY_RENEWAL, 0.0);
    mempool_add(state, &tx, 1.0);

    printf("Policy %s renewed. New expiry: %s", policy_id, ctime(&p->expiry_date));
    return 0;
}

int check_policy_valid_for_claim(ChainState *state, const char *policy_id) {
    Policy *p = find_policy(state, policy_id);
    if (!p) {
        printf("Error: policy %s does not exist.\n", policy_id);
        return 0;
    }

    time_t now = time(NULL);
    if (now > p->expiry_date) {
        p->status = POLICY_EXPIRED;
        printf("Error: policy %s has EXPIRED.\n", policy_id);
        return 0;
    }

    if (p->status == POLICY_EXPIRED) {
        printf("Error: policy %s status is EXPIRED.\n", policy_id);
        return 0;
    }

    return 1;
}
